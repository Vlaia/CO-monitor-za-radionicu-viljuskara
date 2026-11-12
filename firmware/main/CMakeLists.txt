idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES pir buzzer dht11 nvs wifi_manager http_client services)