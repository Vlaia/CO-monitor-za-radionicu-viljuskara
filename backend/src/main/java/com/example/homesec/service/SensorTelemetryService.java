package com.example.homesec.service;

import com.example.homesec.controller.model.SensorTelemetryDTO;
import com.example.homesec.persistence.SensorTelemetry;
import com.example.homesec.persistence.SensorType;
import com.example.homesec.repository.SensorTelemetryDAO;
import jakarta.annotation.Resource;
import lombok.AllArgsConstructor;
import org.springframework.stereotype.Service;

@Service
@AllArgsConstructor
public class SensorTelemetryService {

    @Resource
    private SensorTelemetryDAO sensorTelemetryDAO;

    public SensorTelemetry save(SensorTelemetryDTO sensorTelemetryDTO) {
        SensorTelemetry sensorTelemetry = SensorTelemetry.builder()
                .licence(sensorTelemetryDTO.getLicence())
                .sensorType(SensorType.values()[sensorTelemetryDTO.getSensorType()])
                .humidity(sensorTelemetryDTO.getHumidity())
                .temperature(sensorTelemetryDTO.getTemperature())
                .motionDetected(sensorTelemetryDTO.getMotionDetected())
                .build();

        return sensorTelemetryDAO.save(sensorTelemetry);
    }

}
