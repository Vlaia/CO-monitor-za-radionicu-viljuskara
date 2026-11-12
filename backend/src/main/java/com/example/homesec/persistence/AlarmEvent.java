package com.example.homesec.persistence;

import jakarta.persistence.*;
import lombok.*;

import java.time.ZonedDateTime;

/**
 * Ukljucenje ili iskljucenje alarma na uredjaju (izvor: CO ili pokret).
 */
@AllArgsConstructor
@NoArgsConstructor
@Builder
@Getter
@Setter
@Entity(name = "alarm_event")
public class AlarmEvent {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(nullable = false, name = "id")
    private Long id;

    @Column(name = "device_id", nullable = false)
    private String deviceId;

    @Column(nullable = false)
    private String source;

    @Column(nullable = false)
    private Boolean active;

    @Column(name = "co_ppm")
    private Float coPpm;

    @Column(nullable = false)
    private ZonedDateTime created;

    @PrePersist
    public void setDates() {
        if (getCreated() == null) {
            setCreated(ZonedDateTime.now());
        }
    }
}
