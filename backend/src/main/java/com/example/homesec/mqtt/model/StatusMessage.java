package com.example.homesec.mqtt.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/** radiona/&lt;id&gt;/status (retained): {"online":true,"fw":"0.1.0","ip":"192.168.1.50"} */
@JsonIgnoreProperties(ignoreUnknown = true)
public record StatusMessage(Boolean online, String fw, String ip) {
}
