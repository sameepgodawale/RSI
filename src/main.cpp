#include <Arduino.h>
#include "sim800.h"
#include "loradrv.h"
#include "sd_store.h"
#include "sd_retry.h"
#include "rsu_config.h"
#include "web_config.h"
#include <ArduinoJson.h>

// Pin mapping
#define SIM800_RX_PIN 16
#define SIM800_TX_PIN 17

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_NSS 5
#define LORA_RST 14
#define LORA_DIO0 26

#define SD_CS_PIN 15

TaskHandle_t simTaskHandle = NULL;
TaskHandle_t sdRetryHandle = NULL;
TaskHandle_t loraHandle = NULL;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("RSU ESP32 boot (with web-config & verbose AT logging)");

  // init runtime config (loads NVS)
  config_init();
  Serial.printf("[CFG] RSU API Key: %s\n", cfg_get_rsu_api_key());
  Serial.printf("[CFG] CMS URL: %s\n", cfg_get_cms_url());
  Serial.printf("[CFG] APN: %s\n", cfg_get_apn());

  // Start web configuration portal (AP + web server)
  web_config_start();

  // Initialize SD
  if (!sd_init(SD_CS_PIN)) {
    Serial.println("SD init failed - continuing, will enqueue to SPIFFS (not implemented)");
  }

  // init SIM800 task
  sim800_init(SIM800_RX_PIN, SIM800_TX_PIN);
  xTaskCreatePinnedToCore(sim800_task, "SIMTask", 8192, NULL, 2, &simTaskHandle, 1);

  // start SD retry
  xTaskCreatePinnedToCore(sd_retry_task, "SDRetry", 8192, NULL, 1, &sdRetryHandle, 1);

  // init LoRa
  lora_init(LORA_NSS, LORA_RST, LORA_DIO0, LORA_SCK, LORA_MISO, LORA_MOSI);
  xTaskCreatePinnedToCore(lora_rx_task, "LoRaRX", 8192, NULL, 1, &loraHandle, 1);
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    Serial.println("RSU heartbeat");
  }
  delay(1000);
}
