package com.example.homesec.mqtt.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/** radiona/&lt;id&gt;/alarm: {"source":"co","active":true,"ppm":123.4,"ts":1731430000} */
@JsonIgnoreProperties(ignoreUnknown = true)
public record AlarmMessage(String source, Boolean active, Float ppm, Long ts) {
}
