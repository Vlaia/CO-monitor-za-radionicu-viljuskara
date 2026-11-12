package com.example.homesec.config;

import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.dao.DataAccessException;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Component;

/**
 * Pretvara tabelu sensor_telemetry u TimescaleDB hypertable (na vezbama se ovo radilo rucno).
 * Pokrece se posle Hibernate-a, koji pravi tabelu; sve naredbe su idempotentne.
 */
@Component
@Slf4j
public class TimescaleInitializer implements ApplicationRunner {

    private final JdbcTemplate jdbcTemplate;

    public TimescaleInitializer(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    @Override
    public void run(ApplicationArguments args) {
        try {
            jdbcTemplate.execute("CREATE EXTENSION IF NOT EXISTS timescaledb");
            // Jedinstveni indeks u hypertable-u mora da sadrzi kolonu po kojoj se particionise,
            // pa primarni kljuc samo po id-u mora da se ukloni.
            jdbcTemplate.execute("ALTER TABLE sensor_telemetry DROP CONSTRAINT IF EXISTS sensor_telemetry_pkey");
            jdbcTemplate.execute("SELECT create_hypertable('sensor_telemetry', 'created', "
                    + "if_not_exists => TRUE, migrate_data => TRUE)");
            log.info("sensor_telemetry je TimescaleDB hypertable");
        } catch (DataAccessException e) {
            log.warn("Hypertable nije napravljen (da li je baza TimescaleDB?): {}", e.getMostSpecificCause().getMessage());
        }
    }
}
