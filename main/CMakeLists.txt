idf_component_register(SRCS "main.c" "publisher_tasks.c" "led_bar.c"
                       INCLUDE_DIRS "."
                       REQUIRES wifi_manager mqtt_manager adc_sensor dht11_sensor nvs_flash esp_http_server
                       EMBED_TXTFILES "dashboard.html")