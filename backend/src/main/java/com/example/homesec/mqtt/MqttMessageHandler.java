package com.example.homesec.mqtt;

import com.example.homesec.config.MqttProperties;
import com.example.homesec.mqtt.model.AlarmMessage;
import com.example.homesec.mqtt.model.CoMessage;
import com.example.homesec.mqtt.model.MotionMessage;
import com.example.homesec.mqtt.model.StatusMessage;
import com.example.homesec.service.AlarmEventService;
import com.example.homesec.service.SensorTelemetryService;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;
import tools.jackson.core.JacksonException;
import tools.jackson.databind.ObjectMapper;

import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;

/**
 * Obrada poruka sa tema &lt;prefiks&gt;/&lt;id uredjaja&gt;/{co,pokret,alarm,status}.
 */
@Component
@Slf4j
public class MqttMessageHandler {

    private final ObjectMapper objectMapper;
    private final SensorTelemetryService sensorTelemetryService;
    private final AlarmEventService alarmEventService;
    private final String topicPrefix;

    public MqttMessageHandler(ObjectMapper objectMapper,
                              SensorTelemetryService sensorTelemetryService,
                              AlarmEventService alarmEventService,
                              MqttProperties mqttProperties) {
        this.objectMapper = objectMapper;
        this.sensorTelemetryService = sensorTelemetryService;
        this.alarmEventService = alarmEventService;
        this.topicPrefix = mqttProperties.topicPrefix();
    }

    public void handle(String topic, byte[] payload) {
        String[] parts = topic.split("/");

        if (parts.length != 3 || !parts[0].equals(topicPrefix)) {
            log.warn("Nepoznata tema: {}", topic);
            return;
        }

        String deviceId = parts[1];
        String kind = parts[2];

        try {
            switch (kind) {
                case "co" -> handleCo(deviceId, objectMapper.readValue(payload, CoMessage.class));
                case "pokret" -> handleMotion(deviceId, objectMapper.readValue(payload, MotionMessage.class));
                case "alarm" -> handleAlarm(deviceId, objectMapper.readValue(payload, AlarmMessage.class));
                case "status" -> handleStatus(deviceId, objectMapper.readValue(payload, StatusMessage.class));
                default -> log.debug("Tema {} se ne obradjuje", topic);
            }
        } catch (JacksonException e) {
            log.warn("Neispravan JSON na temi {}: {}", topic, e.getOriginalMessage());
        }
    }

    private void handleCo(String deviceId, CoMessage msg) {
        if (msg.ppm() == null) {
            log.warn("[{}] CO poruka bez vrednosti ppm", deviceId);
            return;
        }

        sensorTelemetryService.saveCo(deviceId, msg.ppm(), msg.mv(), toTime(msg.ts()));
        log.info("[{}] CO {} ppm", deviceId, msg.ppm());
    }

    private void handleMotion(String deviceId, MotionMessage msg) {
        if (msg.motion() == null) {
            log.warn("[{}] Poruka o pokretu bez polja motion", deviceId);
            return;
        }

        sensorTelemetryService.saveMotion(deviceId, msg.motion(), toTime(msg.ts()));
        log.info("[{}] Pokret: {}", deviceId, msg.motion() ? "pocetak" : "kraj");
    }

    private void handleAlarm(String deviceId, AlarmMessage msg) {
        if (msg.source() == null || msg.active() == null) {
            log.warn("[{}] Nepotpuna poruka o alarmu", deviceId);
            return;
        }

        alarmEventService.save(deviceId, msg.source(), msg.active(), msg.ppm(), toTime(msg.ts()));
        log.warn("[{}] ALARM {} ({}): {} ppm", deviceId, msg.active() ? "UKLJUCEN" : "iskljucen",
                msg.source(), msg.ppm());
    }

    private void handleStatus(String deviceId, StatusMessage msg) {
        log.info("[{}] Status: {} (fw {}, ip {})", deviceId,
                Boolean.TRUE.equals(msg.online()) ? "online" : "offline", msg.fw(), msg.ip());
    }

    /** Uredjaj salje Unix vreme ako mu je sat sinhronizovan; inace se koristi vreme prijema. */
    static ZonedDateTime toTime(Long ts) {
        if (ts == null || ts <= 0) {
            return ZonedDateTime.now();
        }
        return ZonedDateTime.ofInstant(Instant.ofEpochSecond(ts), ZoneId.systemDefault());
    }
}
