#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    float latitude;
    float longitude;
    float altitude;
    int satellites;
    bool valid;
} gps_data_t;

void gps_sensor_init(void);
bool gps_get_data(gps_data_t *data);
void gps_reading_task(void *pvParameters);

#endif // GPS_SENSOR_H