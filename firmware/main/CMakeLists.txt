idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES storage wifi_manager mqtt_link services buzzer pir mq7 co_logic
                             esp_netif freertos)
