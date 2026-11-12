idf_component_register(SRCS "mqtt_link.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES mqtt esp_netif esp_app_format)
