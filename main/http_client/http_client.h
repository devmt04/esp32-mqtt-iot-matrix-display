#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))

char *init_http_client(void);

void http_get_datetime(void);
int http_get_weather(void);
void http_get_class_info(void);

#endif