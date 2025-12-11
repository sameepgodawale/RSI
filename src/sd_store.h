#ifndef SD_STORE_H
#define SD_STORE_H

#include <Arduino.h>

bool sd_init(int cs_pin);
int sd_enqueue_eam(const char* json);

#endif
