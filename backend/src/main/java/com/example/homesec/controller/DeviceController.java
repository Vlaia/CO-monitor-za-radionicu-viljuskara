package com.example.homesec.controller;

import com.example.homesec.persistence.AlarmEvent;
import com.example.homesec.persistence.SensorTelemetry;
import com.example.homesec.persistence.SensorType;
import com.example.homesec.service.AlarmEventService;
import com.example.homesec.service.SensorTelemetryService;
import jakarta.annotation.Resource;
import org.springframework.web.bind.annotation.*;

import java.time.ZonedDateTime;
import java.util.List;

/**
 * Citanje sacuvanih podataka. Uredjaj vise ne salje podatke HTTP-om nego preko MQTT-a.
 */
@RestController
@RequestMapping("/device")
public class DeviceController {

    @Resource
    private SensorTelemetryService sensorTelemetryService;

    @Resource
    private AlarmEventService alarmEventService;

    public record CoPoint(ZonedDateTime time, Float ppm, Integer mv) {
    }

    public record MotionPoint(ZonedDateTime time, Boolean motion) {
    }

    public record AlarmEntry(ZonedDateTime time, String source, Boolean active, Float ppm) {
    }

    @GetMapping("/{deviceId}/co")
    public List<CoPoint> co(@PathVariable String deviceId,
                            @RequestParam(defaultValue = "60") int minutes) {
        List<SensorTelemetry> rows = sensorTelemetryService.findSince(
                deviceId, SensorType.MQ7, ZonedDateTime.now().minusMinutes(minutes));

        return rows.stream()
                .map(t -> new CoPoint(t.getCreated(), t.getCoPpm(), t.getCoMv()))
                .toList();
    }

    @GetMapping("/{deviceId}/pokret")
    public List<MotionPoint> motion(@PathVariable String deviceId,
                                    @RequestParam(defaultValue = "24") int hours) {
        List<SensorTelemetry> rows = sensorTelemetryService.findSince(
                deviceId, SensorType.PIR, ZonedDateTime.now().minusHours(hours));

        return rows.stream()
                .map(t -> new MotionPoint(t.getCreated(), t.getMotionDetected()))
                .toList();
    }

    @GetMapping("/{deviceId}/alarm")
    public List<AlarmEntry> alarms(@PathVariable String deviceId,
                                   @RequestParam(defaultValue = "24") int hours) {
        List<AlarmEvent> rows = alarmEventService.findSince(deviceId, ZonedDateTime.now().minusHours(hours));

        return rows.stream()
                .map(a -> new AlarmEntry(a.getCreated(), a.getSource(), a.getActive(), a.getCoPpm()))
                .toList();
    }
}
