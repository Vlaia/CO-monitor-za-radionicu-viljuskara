package com.example.homesec.persistence;

import jakarta.persistence.*;
import lombok.*;

import java.time.ZonedDateTime;

@AllArgsConstructor
@NoArgsConstructor
@Builder
@Getter
@Setter
@Entity(name = "sensor_telemetry")
public class SensorTelemetry {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(unique = true, nullable = false, name = "id")
    private Long id;

    @Column
    private String licence;

    @Column(name = "sensor_type")
    @Enumerated(EnumType.ORDINAL)
    private SensorType sensorType;

    @Column
    private Float temperature;

    @Column
    private Float humidity;

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
