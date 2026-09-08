idf_component_register(SRCS "dht11_sensor.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver freertos esp_timer esp_rom)
