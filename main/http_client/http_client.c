#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"

#include "stdio.h"
#include "http_client.h"
#include "esp_tls.h"

#include "cJSON.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
// #if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
// #endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@lpu.in");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

void http_get_datetime(){
    // To be implemented
}

void http_get_weather(){
    // To be implemented
}

void http_get_class_info(){
    // To be implemented
}

static void http_rest_task(void *pvParameters){    
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
    esp_http_client_config_t config = {
        // .host = CONFIG_EXAMPLE_HTTP_ENDPOINT,
        // .path = "/get",
        // .query = "esp",
        .url = "http://jsonplaceholder.typicode.com/posts/1",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,   // Pass address of local buffer to get response
                                              // or pass the global buffer pointer
        .disable_auto_redirect = true,
        // .skip_cert_common_name_check = true,
    };

    ESP_LOGI(TAG, "HTTP request with url =>");
    esp_http_client_handle_t client = esp_http_client_init(&config);



    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));




    cJSON *root = cJSON_Parse(local_response_buffer);
   if (!root) {
        printf("JSON Parse Error! Raw response: %s\n", local_response_buffer);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    cJSON *origin = cJSON_GetObjectItem(root, "title");

    if (cJSON_IsString(origin))
        printf("title: %s\n", origin->valuestring);

    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}


esp_err_t init_http_client(void){
    xTaskCreate(&http_rest_task, "http_rest_task", 8192, NULL, 5, NULL);
    return ESP_OK;
}


// ADD RETRY AND TIMEOUT MECHANISM
// ADD HTTPS SUPPORT
// ADD REDIRECTION HANDLING
// PARSE JSON RESPONSE
// IMPROVE MEMORY MANAGEMENT
// IMPROVE ERROR HANDLING
// ADD CUSTOM HEADERS SUPPORT
// LOGGING AND DEBUGGING INFORMATION
// MAKE URL, PATH, QUERY DYNAMIC VIA FUNCTION PARAMETERS
// ADD SUPPORT FOR OTHER HTTP METHODS (POST, PUT, DELETE)
// ADD ABILITY TO SEND PAYLOAD IN REQUEST BODY FOR METHODS LIKE POST AND PUT
