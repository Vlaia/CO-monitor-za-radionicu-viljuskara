#include "mqtt_link.h"

#include <stdio.h>
#include <string.h>

#include "mqtt_client.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "sdkconfig.h"

#define TOPIC_MAX_LEN 96

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client = NULL;
static volatile bool connected = false;

static char status_topic[TOPIC_MAX_LEN];

static const char LWT_PAYLOAD[] = "{\"online\":false}";

static void build_topic(char *buf, size_t len, const char *subtopic)
{
    snprintf(buf, len, "%s/%s/%s", CONFIG_MQTT_TOPIC_PREFIX, CONFIG_DEVICE_ID, subtopic);
}

static void publish_online_status(void)
{
    char ip[16] = "";
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;

    if (netif != NULL && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_info.ip));
    }

    char payload[128];
    int len = snprintf(payload, sizeof(payload),
                       "{\"online\":true,\"fw\":\"%s\",\"ip\":\"%s\"}",
                       esp_app_get_description()->version, ip);

    esp_mqtt_client_publish(client, status_topic, payload, len, 1, 1);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        connected = true;
        ESP_LOGI(TAG, "Povezan na broker %s", CONFIG_MQTT_BROKER_URI);
        publish_online_status();
        break;

    case MQTT_EVENT_DISCONNECTED:
        connected = false;
        ESP_LOGW(TAG, "Veza sa brokerom prekinuta, ponovno povezivanje...");
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT greska");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_link_start(void)
{
    if (client != NULL) {
        return ESP_OK;
    }

    build_topic(status_topic, sizeof(status_topic), "status");

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
        .credentials.client_id = CONFIG_DEVICE_ID,
        .session.keepalive = 30,
        .session.last_will = {
            .topic = status_topic,
            .msg = LWT_PAYLOAD,
            .msg_len = sizeof(LWT_PAYLOAD) - 1,
            .qos = 1,
            .retain = 1,
        },
    };

    client = esp_mqtt_client_init(&cfg);

    if (client == NULL) {
        ESP_LOGE(TAG, "Ne mogu da kreiram MQTT klijent");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                                   mqtt_event_handler, NULL);

    if (err == ESP_OK) {
        err = esp_mqtt_client_start(client);
    }

    return err;
}

bool mqtt_link_is_connected(void)
{
    return connected;
}

esp_err_t mqtt_link_publish(const char *subtopic, const char *json, int qos, bool retain)
{
    if (client == NULL || subtopic == NULL || json == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    char topic[TOPIC_MAX_LEN];
    build_topic(topic, sizeof(topic), subtopic);

    int msg_id;

    if (qos > 0) {
        // Ne blokira; poruka ostaje u outbox-u dok je broker ne potvrdi.
        msg_id = esp_mqtt_client_enqueue(client, topic, json, 0, qos, retain, true);
    } else {
        if (!connected) {
            return ESP_ERR_INVALID_STATE;
        }
        msg_id = esp_mqtt_client_publish(client, topic, json, 0, qos, retain);
    }

    if (msg_id < 0) {
        ESP_LOGW(TAG, "Slanje na %s nije uspelo", topic);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "%s <- %s", topic, json);

    return ESP_OK;
}
