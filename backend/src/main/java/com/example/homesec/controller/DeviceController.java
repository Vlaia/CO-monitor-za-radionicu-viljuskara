package com.example.homesec.controller;

import com.example.homesec.exception.HomesecException;
import com.example.homesec.controller.model.SensorTelemetryDTO;
import com.example.homesec.service.SensorTelemetryService;
import jakarta.annotation.Resource;
import lombok.extern.slf4j.Slf4j;
import org.antlr.v4.runtime.misc.NotNull;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/device")
@Slf4j
public class DeviceController {

    @Resource
    private SensorTelemetryService sensorTelemetryService;

    @PostMapping("/sensor-telemetry")
    public ResponseEntity<String> createSensorTelemetry(@NotNull @RequestBody SensorTelemetryDTO sensorTelemetryDTO) {
        log.info("POST New sensor telemetry for device with licence: " + sensorTelemetryDTO.getLicence());
        try {
            sensorTelemetryService.save(sensorTelemetryDTO);
            return ResponseEntity.ok("DONE");
        } catch (HomesecException hse) {
            log.warn("POST Sensor Telemetry not created HomesecException: {}", hse.getMessage());
            return ResponseEntity.badRequest().body(hse.getMessage());
        }
    }
}
