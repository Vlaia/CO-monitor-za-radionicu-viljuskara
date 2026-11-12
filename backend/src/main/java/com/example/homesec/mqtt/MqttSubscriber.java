package com.example.homesec.mqtt;

import com.example.homesec.config.MqttProperties;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.*;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.springframework.beans.factory.DisposableBean;
import org.springframework.boot.context.event.ApplicationReadyEvent;
import org.springframework.context.event.EventListener;
import org.springframework.stereotype.Component;

/**
 * Pretplata na teme uredjaja na MQTT brokeru. Ako broker nije dostupan pri pokretanju,
 * povezivanje se ponavlja u pozadini; posle prvog uspeha Paho se sam ponovo povezuje.
 */
@Component
@Slf4j
public class MqttSubscriber implements MqttCallbackExtended, DisposableBean {

    private static final long RETRY_MS = 5000;
    private static final String[] SUBTOPICS = {"co", "pokret", "alarm", "status"};

    private final MqttProperties properties;
    private final MqttMessageHandler handler;

    private volatile MqttClient client;
    private volatile boolean running = true;

    public MqttSubscriber(MqttProperties properties, MqttMessageHandler handler) {
        this.properties = properties;
        this.handler = handler;
    }

    @EventListener(ApplicationReadyEvent.class)
    public void start() {
        Thread thread = new Thread(this::connectLoop, "mqtt-connect");
        thread.setDaemon(true);
        thread.start();
    }

    private void connectLoop() {
        try {
            client = new MqttClient(properties.brokerUrl(), properties.clientId(), new MemoryPersistence());
            client.setCallback(this);
        } catch (MqttException e) {
            log.error("Neispravna MQTT podesavanja: {}", e.getMessage());
            return;
        }

        MqttConnectOptions options = new MqttConnectOptions();
        options.setAutomaticReconnect(true);
        options.setCleanSession(true);
        options.setConnectionTimeout(10);
        options.setKeepAliveInterval(30);

        while (running && !client.isConnected()) {
            try {
                client.connect(options);
            } catch (MqttException e) {
                log.warn("Broker {} nije dostupan ({}), novi pokusaj za {} s",
                        properties.brokerUrl(), e.getMessage(), RETRY_MS / 1000);
                try {
                    Thread.sleep(RETRY_MS);
                } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    return;
                }
            }
        }
    }

    @Override
    public void connectComplete(boolean reconnect, String serverURI) {
        log.info("{} na MQTT broker {}", reconnect ? "Ponovo povezan" : "Povezan", serverURI);

        // Sa cleanSession=true pretplate se gube pri prekidu, pa se obnavljaju svaki put.
        try {
            for (String subtopic : SUBTOPICS) {
                String filter = properties.topicPrefix() + "/+/" + subtopic;
                client.subscribe(filter, 1);
                log.info("Pretplacen na {}", filter);
            }
        } catch (MqttException e) {
            log.error("Pretplata nije uspela: {}", e.getMessage());
        }
    }

    @Override
    public void connectionLost(Throwable cause) {
        log.warn("Veza sa MQTT brokerom prekinuta: {}", cause.getMessage());
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) {
        // Izuzetak iz ove metode bi prekinuo vezu sa brokerom, pa se sve hvata ovde.
        try {
            handler.handle(topic, message.getPayload());
        } catch (Exception e) {
            log.error("Greska pri obradi poruke sa teme {}", topic, e);
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // Backend za sada ne salje poruke.
    }

    @Override
    public void destroy() throws Exception {
        running = false;

        if (client != null) {
            if (client.isConnected()) {
                client.disconnect();
            }
            client.close();
        }
    }
}
