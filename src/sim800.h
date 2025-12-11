#ifndef SIM800_H
#define SIM800_H

#include <Arduino.h>

void sim800_init(int rx_pin, int tx_pin);
void sim800_task(void *pvParameters);
int sim800_http_post_async(const char* url, const char* json, const char* header_key, const char* header_value);
int sim800_http_post_block(const char* url, const char* json, const char* header_key, const char* header_value);

// helper to force GPRS/APN configuration
int sim800_force_gprs_setup(void);

#endif
