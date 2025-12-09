#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct _weather_data_t {
    int temp;
    int wind_speed;
} weather_data_t;

weather_data_t http_get_weather(void);

#endif