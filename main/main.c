#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

#include "wifi_sta/wifi_sta.h"
#include "http_client/http_client.h"
#include "MAX7219/MAX7219.h"

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/apps/sntp.h"

#include <string.h>
#include <ctype.h>
#include <esp_netif_sntp.h>

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

static void init_ntp(void){
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    setenv("TZ", "IST-5:30", 1);
    tzset();
    esp_netif_sntp_init(&config);
    
    while (1){
        esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)); // wait up to 15 sec

        if (ret == ESP_OK) {
            ESP_LOGI("SNTP", "Time synchronized successfully!");
            break;
        }

        ESP_LOGW("SNTP", "NTP sync failed. Retrying in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI("TIME", "The updated current time is: %s", asctime(&timeinfo));
}


void display_task(void *pvParameters) {
    // Request Datetime
    // Request Weatherq
    // Request Class Info

    while (1){
        // Update data on display
        char *d = init_http_client();
        if(d!=NULL){
            for (int i = 0; i < strlen(d); i++)
                d[i] = toupper(d[i]);
    
            test_draw(d);
            free(d);
        }else{
            test_draw("ERR ");
            ESP_LOGE("MAIN", "Failed to get data from HTTP client");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }   
}

static void engine_task(void *pvParameters){
    // init Network interface
    init_nvs_netif();

    // init SPI for MAX7219
    init_spi();
    // Draw on Display
    test_draw("INIT");

    // init WiFi
    if(wifi_init_sta() != ESP_OK){
        ESP_LOGE("MAIN", "WiFi initialization unexpectedly Failed!");
    }    
    init_ntp();

    int weather_counter = -1; // -1 as initial value to force first update
    ESP_LOGI("DISPLAY", "Starting display...");
    int temp = 0;
    int hr = 0; 
    int min = 0;
    int sec = 0;
    while(1){
        // Update Time(HH:MM) Update every 15 seconds
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        hr   = timeinfo.tm_hour;   // eg: 13
        min = timeinfo.tm_min;    // eg: 47
        sec = timeinfo.tm_sec;    

        //Update Weather every 10 minutes(600 seconds) - can use a timer or counter
        if(weather_counter == -1 || weather_counter >= 40){ // 600 sec / 15 sec = 40 sec
            // Check in case WIFI is disconnected in between
            // Update Weather Info
            // Make HTTP Request to get weather info in seprate TASK, then return result from it, retry if fails
            temp = http_get_weather();
            if(temp != -1){
                ESP_LOGI("HTTP","GET Temp : %d\n", temp);
            }else{
                ESP_LOGE("HTTP", "Failed to get weather data");
            }
            weather_counter = 0;
        }else {
            weather_counter++;
        }
            

        // Display weather and time
        if(temp >=0){
            draw_time_weather(temp, hr, sec);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 15000
    }
}

void app_main(void){
    xTaskCreate(engine_task, "engine_task", 8192, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
}