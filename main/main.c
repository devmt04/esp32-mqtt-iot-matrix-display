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
#include <esp_netif_sntp.h>
#include "freertos/semphr.h"
#include "MQTT/MQTT.h"

#include <string.h>
#include <ctype.h>

SemaphoreHandle_t mqtt_mutex;

static void init_nvs_netif(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
}

static void init_ntp(void){
    // TODO : IMPLIMENT AN EXTERNAL RTC
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

static void push_col(uint8_t buf[32], uint8_t col, int buflen){
    // For each of the 8 rows (top to bottom)
    for (int row = 0; row < 8; row++)
    {
        // Compute the 4 module row indices
        int i0 = row;        // module 4
        int i1 = 8 + row;    // module 5
        int i2 = 16 + row;   // module 6
        int i3 = 24 + row;   // module 7

        // Save MSB bits for cascading
        uint8_t msb0 = (buf[i0] & 0x80) >> 7;
        uint8_t msb1 = (buf[i1] & 0x80) >> 7;
        uint8_t msb2 = (buf[i2] & 0x80) >> 7;

        // 1) Shift LEFT module 4 and take MSB from module 5
        buf[i0] <<= 1;
        buf[i0] |= msb1;

        // 2) Shift LEFT module 5 and take MSB from module 6
        buf[i1] <<= 1;
        buf[i1] |= msb2;

        // 3) Shift LEFT module 6 and take MSB from module 7
        uint8_t msb3 = (buf[i3] & 0x80) >> 7;
        buf[i2] <<= 1;
        buf[i2] |= msb3;

        // 4) Shift LEFT module 7 and insert NEW COLUMN BIT
        buf[i3] <<= 1;
        uint8_t new_bit = (col >> row) & 1;
        buf[i3] |= new_bit;
    }
}


static void display_msg_task(void *pvParameters){
    // if msg not overflowed -> showed it statically
        // if msg overflowed -> then
            // Algorithim 01:
                // create a blank 8x32 sized buffer -> buf1
                // create a another buffer that can fully contain the text provided -> buf2
                // slide buf2 over buf1, from right to left, one column at a time
                // when buf2's last col reached buf1's first col, start the from again

            // Algorithim 02:
                // maintain a 8x32 buffre -> buf  
                // exract each charcters from text -> they sized 8x8 (rows x cols)
                // crop each charcters to fit into 8x5
                // for each charcter array -> cbuf : 
                    // maintain a col_crsr, that hold valu from 0 to 4 (for 5 cols)
                    // at col_crsr, 
                        // shift the buf left one step
                        // push the cbuf[col_crsr] at the end of buf
                        // increment col_crsr by 1, 
                        // if col_crsr >= 5
                        // push a 2 col gap in buf
                        // break the loop at col_crsr = 4
                        // col_crsr = 0
                        // new cbuf will bew allocated
                        // Now push blank 8x16 blank data into buf, to flush the text out
                        // continuing update buf similary

    char *msg = mqtt_msg.msg;
    int msglen = strlen(msg);
    int speed = 80;                    
    uint8_t buf[32] = {0};
    
    while (1) {
        // Check for any update in mqtt_msg and then proceed
        xSemaphoreTake(mqtt_mutex, portMAX_DELAY);
        if(mqtt_data_update){
            set_all_brightness(mqtt_msg.intensity);
            strncpy(msg, mqtt_msg.msg, strlen(mqtt_msg.msg));
            // strcpy(local_msg_buffer, mqtt_msg.msg);
            // msg = local_msg_buffer;
            msglen = strlen(msg);
            mqtt_data_update = false;
        }
        // draw charcters on buf, and then shift it
        printf("---> intensity: %d", mqtt_msg.intensity);
        xSemaphoreGive(mqtt_mutex);

        for(int i = 0; i<msglen; i++){
            char c = msg[i];
            uint8_t cbuf[5];
            printf("--> %c\n", c);
            if(c == ' ') memcpy(cbuf, string_font6x5[26].rows, 5);
            else if(c == '!') memcpy(cbuf, string_font6x5[27].rows, 5);
            else if(c == '.') memcpy(cbuf, string_font6x5[28].rows, 5);
            else memcpy(cbuf, string_font6x5[c - 'A'].rows, 5);

            for(int col = 0; col<5; col++){
                push_col(buf, cbuf[col], 32);
                draw_buffer(buf);
                vTaskDelay(pdMS_TO_TICKS(speed));
            }
            // push 2 blank space
            for(int _ = 0; _< 2; _++){
                push_col(buf, 0b00000000, 32);
                draw_buffer(buf);
                vTaskDelay(pdMS_TO_TICKS(speed));
            }
        }
        
        // as we reahced to the end of msg, insert 16 blank spaces in between
        for(int _ = 0; _< 16; _++){
            push_col(buf, 0b00000000, 32);
            draw_buffer(buf);
            vTaskDelay(pdMS_TO_TICKS(speed));
        }
    }
}


void display_time_task(void *pvParameters){
    int hr = 0; 
    int min = 0;
    int sec = 0;

    while(1){
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        hr   = timeinfo.tm_hour; 
        min = timeinfo.tm_min;  
        sec = timeinfo.tm_sec;    
        draw_time(hr, min, sec);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void display_weather_task(void *pvParameters){
    weather_data_t weather_data;
    while(1){
        weather_data = http_get_weather();
        if(weather_data.temp != -1){
            ESP_LOGI("HTTP","GET Temp : %d\n", weather_data.temp);
        }else{
            ESP_LOGE("HTTP", "Failed to get weather data");
            continue;
        }
        if(weather_data.wind_speed != -1){
            ESP_LOGI("HTTP","GET Wind Speed : %d\n", weather_data.wind_speed);
        }else{
            ESP_LOGE("HTTP", "Failed to get weather data");
            continue;
        }

        if(weather_data.temp >0 && weather_data.wind_speed>=0){
            // TODO : Handle -ve temperature
            draw_weather(weather_data);
        }
        vTaskDelay(pdMS_TO_TICKS(600000)); // 10 min
    }
}

static void engine_task(void *pvParameters){
    // init Network interface
    init_nvs_netif();
    // init SPI for MAX7219
    init_spi();
    // Draw on Display
    set_all_brightness(0x00); // 0x00 -> MIN, 0x0F -> MAX, 0x08 -> 50%
    draw_init();

    // init WiFi
    if(wifi_init_sta() != ESP_OK){
        ESP_LOGE("MAIN", "WiFi initialization unexpectedly Failed!");
    }
    
    ESP_LOGI("DISPLAY", "Starting display...");
    
    xTaskCreate(display_msg_task, "display_msg_task", 6144, NULL, 5, NULL);
    init_ntp();

    xTaskCreate(display_time_task, "display_time_task", 4096, NULL, 5, NULL);
    xTaskCreate(display_weather_task, "display_weather_task", 6144, NULL, 5, NULL);

    if(mqtt_init() != ESP_OK){
        ESP_LOGE("MQTT", "Failed to initilize.");
    }

    while(1){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void){
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    mqtt_mutex = xSemaphoreCreateMutex();
    xTaskCreate(engine_task, "engine_task", 8192, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
}