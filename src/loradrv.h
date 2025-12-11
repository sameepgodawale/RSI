#ifndef LORADRV_H
#define LORADRV_H

#include <Arduino.h>

bool lora_init(int cs, int rst, int dio0, int sck, int miso, int mosi);
int lora_receive(char* buf, int maxlen, int timeout_ms);
int lora_send_ack(const char* addr, const char* msg);

void lora_rx_task(void *pvParameters);

#endif
