#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"

// #include <stdint.h>
#include <string.h>
#include "stdio.h"
#include <stdbool.h>
#include "MQTT.h"

#define TAG "MQTT"

TaskHandle_t heartbeat_task_handle = NULL;
bool heartbeat_started = false;

mqtt_msg_t mqtt_msg = {
    .intensity = 15,
    .msg = "HELLO LPU! WE ARE CIRCUIT CRAFTERS."
};

volatile bool mqtt_data_update = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqtt_heartbeat_task(void *arg){
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)arg;
    while (heartbeat_started) {
        esp_mqtt_client_publish(client,
            "/classplate/status/device1",
            "{\"online\":true}",
            0, 1, 0);

        vTaskDelay(pdMS_TO_TICKS(10000));  // every 10 seconds
    }
    heartbeat_task_handle = NULL;
    vTaskDelete(NULL);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // msg_id = esp_mqtt_client_publish(client, "/classplate/status", "data_3", 0, 1, 0);
            
            msg_id = esp_mqtt_client_publish(client, "/classplate/status/device1", "{\"online\":true}", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            // esp_mqtt_client_subscribe(client, "/classplate/message/device1", 0);
            // esp_mqtt_client_subscribe(client, "/classplate/intensity/device1", 1);
            msg_id = esp_mqtt_client_subscribe(client, "/classplate/message/device1", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/classplate/intensity/device1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // Start heartbeat task ONLY once
            if (!heartbeat_started) {
                heartbeat_started = true;
                xTaskCreate(mqtt_heartbeat_task, "mqtt_heartbeat_task", 4096, 
                    client,      // pass MQTT client handle
                    5, &heartbeat_task_handle);
            }

            // START A TASK THAT WILL CONTINOUSLY SEND ONLINE UPDATE TO WEB CLIENT


            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            if (heartbeat_started) {
                heartbeat_started = false;  // Task will exit its loop
            }
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            // CREATE CALLBACK FUNTIONS HERE
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            
            xSemaphoreTake(mqtt_mutex, portMAX_DELAY);

            mqtt_data_update = true;
            if(strncmp(event->topic, "/classplate/intensity/device1", event->topic_len) == 0) {
                mqtt_msg.intensity = atoi(event->data);
            }
            if(strncmp(event->topic, "/classplate/message/device1", event->topic_len) == 0) {
                strncpy(mqtt_msg.msg, event->data, event->data_len);
                mqtt_msg.msg[event->data_len] = '\0';
            }

            xSemaphoreGive(mqtt_mutex);

            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
        }
}

esp_err_t mqtt_init(void){
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler */
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    esp_err_t ret = esp_mqtt_client_start(client);
    return ret;
}