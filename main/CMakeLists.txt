idf_component_register(SRCS "main.c"
                            "telemetry.c"
                            "webserver.c"
                            "publisher_tasks.c"
                            "led_bar.c"
                            "gps_sensor.c"
                       INCLUDE_DIRS "."
                       REQUIRES wifi_manager mqtt_manager adc_sensor dht11_sensor nvs_flash esp_http_server driver motor_driver
                       EMBED_TXTFILES "dashboard.html")