package com.example.homesec.repository;

import com.example.homesec.persistence.SensorTelemetry;
import com.example.homesec.persistence.SensorType;
import org.springframework.data.repository.CrudRepository;
import org.springframework.stereotype.Repository;

import java.time.ZonedDateTime;
import java.util.List;

@Repository
public interface SensorTelemetryDAO extends CrudRepository<SensorTelemetry, Long> {

    List<SensorTelemetry> findByDeviceIdAndSensorTypeAndCreatedAfterOrderByCreatedAsc(
            String deviceId, SensorType sensorType, ZonedDateTime after);
}
