#ifndef WIFI_STA_H
#define WIFI_STA_H

// https://github.com/espressif/esp-idf/blob/v5.5.1/examples/wifi/getting_started/station/main/station_example_main.c

#define TAG "WIFI_STA"

// #define WIFI_SSID      "LPU Hostels"
// #define WIFI_PASS      "123456789a"

#define WIFI_SSID      "Mohit" // iPhone Hotspot
#define WIFI_PASS      "12345678"

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

esp_err_t wifi_init_sta(void);

#endif