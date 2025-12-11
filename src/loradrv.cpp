#include "loradrv.h"
#include <SPI.h>
#include <LoRa.h>
#include "sim800.h"
#include "sd_store.h"
#include <ArduinoJson.h>

static int cs_pin_global = 5;

bool lora_init(int cs, int rst, int dio0, int sck, int miso, int mosi) {
  SPI.begin(sck, miso, mosi, cs);
  cs_pin_global = cs;
  LoRa.setPins(cs, rst, dio0);
  if (!LoRa.begin(915E6)) { // frequency may need to be changed per region
    Serial.println("LoRa init failed"); return false;
  }
  Serial.println("LoRa ready");
  return true;
}

int lora_receive(char* buf, int maxlen, int timeout_ms) {
  // blocking receive using LoRa.parsePacket with timeout
  unsigned long start = millis();
  while (millis() - start < (unsigned long)timeout_ms) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      int i=0;
      while (LoRa.available() && i < maxlen-1) { buf[i++] = (char)LoRa.read(); }
      buf[i]=0;
      return i;
    }
    delay(5);
  }
  return 0;
}

int lora_send_ack(const char* addr, const char* msg) {
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  return 1;
}

void lora_rx_task(void *pvParameters) {
  (void)pvParameters;
  char buf[512];
  for (;;) {
    int n = lora_receive(buf, sizeof(buf), 1000);
    if (n > 0) {
      Serial.printf("LoRa: received %s\n", buf);
      // simple validation: JSON object starts with '{'
      if (buf[0] == '{') {
        StaticJsonDocument<512> doc;
        auto err = deserializeJson(doc, buf);
        if (!err) {
          sim800_http_post_async(CMS_API_URL, buf, "x-api-key", cfg_get_rsu_api_key());
          lora_send_ack(NULL, "ACK");
        } else {
          // invalid json - ignore or log
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
