package com.example.homesec.config;

import org.springframework.boot.context.properties.ConfigurationProperties;

/**
 * Podesavanja iz application.properties (mqtt.*).
 */
@ConfigurationProperties(prefix = "mqtt")
public record MqttProperties(String brokerUrl, String clientId, String topicPrefix) {
}
