package com.example.homesec.service;

import com.example.homesec.persistence.SensorTelemetry;
import com.example.homesec.persistence.SensorType;
import com.example.homesec.repository.SensorTelemetryDAO;
import jakarta.annotation.Resource;
import lombok.AllArgsConstructor;
import org.springframework.stereotype.Service;

import java.time.ZonedDateTime;
import java.util.List;

@Service
@AllArgsConstructor
public class SensorTelemetryService {

    @Resource
    private SensorTelemetryDAO sensorTelemetryDAO;

    public SensorTelemetry saveCo(String deviceId, float ppm, Integer sensorMv, ZonedDateTime created) {
        SensorTelemetry sensorTelemetry = SensorTelemetry.builder()
                .deviceId(deviceId)
                .sensorType(SensorType.MQ7)
                .coPpm(ppm)
                .coMv(sensorMv)
                .created(created)
                .build();

        return sensorTelemetryDAO.save(sensorTelemetry);
    }

    public SensorTelemetry saveMotion(String deviceId, boolean motion, ZonedDateTime created) {
        SensorTelemetry sensorTelemetry = SensorTelemetry.builder()
                .deviceId(deviceId)
                .sensorType(SensorType.PIR)
                .motionDetected(motion)
                .created(created)
                .build();

        return sensorTelemetryDAO.save(sensorTelemetry);
    }

    public List<SensorTelemetry> findSince(String deviceId, SensorType type, ZonedDateTime since) {
        return sensorTelemetryDAO.findByDeviceIdAndSensorTypeAndCreatedAfterOrderByCreatedAsc(deviceId, type, since);
    }

}
