package com.example.homesec.persistence;

import jakarta.persistence.*;
import lombok.*;

import java.time.ZonedDateTime;

/**
 * Merenja senzora. Tabela je TimescaleDB hypertable particionisana po koloni "created"
 * (vidi TimescaleInitializer).
 */
@AllArgsConstructor
@NoArgsConstructor
@Builder
@Getter
@Setter
@Entity(name = "sensor_telemetry")
public class SensorTelemetry {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(nullable = false, name = "id")
    private Long id;

    @Column(name = "device_id", nullable = false)
    private String deviceId;

    @Column(name = "sensor_type")
    @Enumerated(EnumType.ORDINAL)
    private SensorType sensorType;

    @Column(name = "co_ppm")
    private Float coPpm;

    // Napon na izlazu MQ-7 modula, cuva se radi kasnije kalibracije.
    @Column(name = "co_mv")
    private Integer coMv;

    @Column(name = "motion_detected")
    private Boolean motionDetected;

    @Column(nullable = false)
    private ZonedDateTime created;

    @PrePersist
    @PreUpdate
    public void setDates() {
        ZonedDateTime now = ZonedDateTime.now();
        if (getCreated() == null) {
            setCreated(now);
        }
    }

}
