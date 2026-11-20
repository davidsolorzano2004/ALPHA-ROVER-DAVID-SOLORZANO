idf_component_register(SRCS "mqtt_manager.c"
                       INCLUDE_DIRS "include" "../../main"
                       REQUIRES mqtt esp_event driver log esp_adc)