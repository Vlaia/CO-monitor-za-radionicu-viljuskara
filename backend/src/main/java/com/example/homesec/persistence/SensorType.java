package com.example.homesec.persistence;

import java.util.Arrays;

public enum SensorType {
    DHT11(0),
    PIR(1);

    private final int value;

    SensorType(int value) {
        this.value = value;
    }

    public static SensorType fromValue(Integer value) {
        return Arrays.stream(values())
                .filter(type -> type.value == value)
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException(
                        "Unknown sensor type: " + value
                ));
    }
}
