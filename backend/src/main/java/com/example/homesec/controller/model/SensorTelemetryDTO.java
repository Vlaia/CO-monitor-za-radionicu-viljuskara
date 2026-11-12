package com.example.homesec.controller.model;

import lombok.*;

@AllArgsConstructor
@NoArgsConstructor
@Getter
@Setter
@Builder
public class SensorTelemetryDTO {

    private String licence;
    private Integer sensorType;
    private Float temperature;
    private Float humidity;
    private Boolean motionDetected;
}
