package com.example.homesec.mqtt.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/** radiona/&lt;id&gt;/pokret: {"motion":true,"ts":1731430000} */
@JsonIgnoreProperties(ignoreUnknown = true)
public record MotionMessage(Boolean motion, Long ts) {
}
