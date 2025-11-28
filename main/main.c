#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

#include "wifi_sta/wifi_sta.h"
#include "http_client/http_client.h"

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static void init_nvs_netif(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void app_main(void){
    init_nvs_netif();
    if(wifi_init_sta() == ESP_OK){
        ESP_LOGI("MAIN", "WiFi Initialized Successfully");
    }
    init_http_client();

    // Request Datetime
    // Request Weatherq
    // Request Class Info

    while (1){
        // Update data on display
        vTaskDelay(1000 / portTICK_PERIOD_MS);    
    }   
}