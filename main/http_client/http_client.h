#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#define TAG "HTTP_CLIENT"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

esp_err_t init_http_client(void);

void http_get_datetime();
void http_get_weather();
void http_get_class_info();

#endif