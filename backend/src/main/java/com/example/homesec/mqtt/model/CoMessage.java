package com.example.homesec.mqtt.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/** radiona/&lt;id&gt;/co: {"ppm":12.3,"mv":812,"ts":1731430000} */
@JsonIgnoreProperties(ignoreUnknown = true)
public record CoMessage(Float ppm, Integer mv, Long ts) {
}
