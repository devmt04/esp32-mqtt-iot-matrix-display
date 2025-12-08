#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/semphr.h"

#define MQTT_BROKER_URL "mqtt://broker.hivemq.com:1883"

typedef struct _mqtt_msg_t{
    int intensity;
    char msg[128];
} mqtt_msg_t;

extern mqtt_msg_t mqtt_msg;
extern volatile bool mqtt_data_update;
extern SemaphoreHandle_t mqtt_mutex;

esp_err_t mqtt_init(void);

#endif