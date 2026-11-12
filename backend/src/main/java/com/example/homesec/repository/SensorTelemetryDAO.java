package com.example.homesec.repository;

import com.example.homesec.persistence.SensorTelemetry;
import org.springframework.data.repository.CrudRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface SensorTelemetryDAO extends CrudRepository<SensorTelemetry, Long> {
}
