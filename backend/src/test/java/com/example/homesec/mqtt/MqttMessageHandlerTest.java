package com.example.homesec.mqtt;

import com.example.homesec.config.MqttProperties;
import com.example.homesec.service.AlarmEventService;
import com.example.homesec.service.SensorTelemetryService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import tools.jackson.databind.json.JsonMapper;

import java.nio.charset.StandardCharsets;
import java.time.ZonedDateTime;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

class MqttMessageHandlerTest {

    private SensorTelemetryService telemetryService;
    private AlarmEventService alarmEventService;
    private MqttMessageHandler handler;

    @BeforeEach
    void setUp() {
        telemetryService = mock(SensorTelemetryService.class);
        alarmEventService = mock(AlarmEventService.class);
        handler = new MqttMessageHandler(
                JsonMapper.builder().build(),
                telemetryService,
                alarmEventService,
                new MqttProperties("tcp://localhost:1883", "test", "radiona"));
    }

    private static byte[] json(String s) {
        return s.getBytes(StandardCharsets.UTF_8);
    }

    @Test
    void savesCoMeasurementWithDeviceTimestamp() {
        handler.handle("radiona/radiona1/co", json("{\"ppm\":42.5,\"mv\":812,\"ts\":1731430000}"));

        verify(telemetryService).saveCo(eq("radiona1"), eq(42.5f), eq(812),
                argThat(t -> t.toEpochSecond() == 1731430000L));
    }

    @Test
    void coWithoutTimestampUsesReceiveTime() {
        ZonedDateTime before = ZonedDateTime.now().minusSeconds(1);

        handler.handle("radiona/radiona1/co", json("{\"ppm\":3.0,\"mv\":400}"));

        verify(telemetryService).saveCo(eq("radiona1"), eq(3.0f), eq(400),
                argThat(t -> t.isAfter(before)));
    }

    @Test
    void savesMotionEvent() {
        handler.handle("radiona/r2/pokret", json("{\"motion\":true,\"ts\":1731430000}"));

        verify(telemetryService).saveMotion(eq("r2"), eq(true), any());
    }

    @Test
    void savesAlarmEvent() {
        handler.handle("radiona/radiona1/alarm",
                json("{\"source\":\"co\",\"active\":true,\"ppm\":120.0,\"ts\":1731430000}"));

        verify(alarmEventService).save(eq("radiona1"), eq("co"), eq(true), eq(120.0f), any());
    }

    @Test
    void ignoresForeignTopicsAndBrokenJson() {
        handler.handle("drugo/radiona1/co", json("{\"ppm\":1}"));
        handler.handle("radiona/radiona1/co", json("nije json"));
        handler.handle("radiona/radiona1/co", json("{\"mv\":100}"));
        handler.handle("radiona/radiona1/cmd", json("{}"));

        verifyNoInteractions(telemetryService, alarmEventService);
    }

    @Test
    void zeroTimestampMeansClockNotSynced() {
        assertThat(MqttMessageHandler.toTime(0L)).isAfter(ZonedDateTime.now().minusMinutes(1));
        assertThat(MqttMessageHandler.toTime(1731430000L).toEpochSecond()).isEqualTo(1731430000L);
    }
}
