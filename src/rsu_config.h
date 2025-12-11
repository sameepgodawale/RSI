#ifndef RSU_CONFIG_H
#define RSU_CONFIG_H

#include <Arduino.h>

void config_init(); // load preferences
const char* cfg_get_rsu_api_key();
const char* cfg_get_cms_url();
const char* cfg_get_apn();
const char* cfg_get_apn_user();
const char* cfg_get_apn_pass();
const char* cfg_get_sim_pin();

bool cfg_set_rsu_api_key(const char* v);
bool cfg_set_cms_url(const char* v);
bool cfg_set_apn(const char* v);
bool cfg_set_apn_user(const char* v);
bool cfg_set_apn_pass(const char* v);
bool cfg_set_sim_pin(const char* v);

#endif
