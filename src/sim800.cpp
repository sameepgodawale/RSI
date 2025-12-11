/* sim800.cpp - verbose AT logging + runtime APN config via rsu_config */

#include "sim800.h"
#include "SD.h"
#include "sd_store.h"
#include "rsu_config.h"
#include "include/cms_config.h"
#include <HardwareSerial.h>

#define SIM_SERIAL_NUM 2
static HardwareSerial SimSerial(SIM_SERIAL_NUM);

static QueueHandle_t simQ = NULL;
static SemaphoreHandle_t gprs_mutex = NULL;

static bool gprs_ready = false;
static unsigned long last_gprs_attempt_ms = 0;

void sim800_init(int rx_pin, int tx_pin) {
  SimSerial.begin(115200, SERIAL_8N1, rx_pin, tx_pin);
  if (!simQ) simQ = xQueueCreate(12, sizeof(char*));
  if (!gprs_mutex) gprs_mutex = xSemaphoreCreateMutex();
}

// ----- verbose trace helpers ------------------------------------------------

static void trace_at_cmd(const char* cmd) {
  Serial.printf("[AT ->] %s\r\n", cmd);
}

static void trace_modem_resp(const char* tag, const char* resp) {
  // print up to a reasonable size
  Serial.printf("[AT <- %s] %s\r\n", tag, resp ? resp : "");
}

// ----- low-level AT comms ---------------------------------------------------

static void at_send(const char* cmd) {
  trace_at_cmd(cmd);
  SimSerial.print(cmd);
  SimSerial.print("\r\n");
}

// read a chunk of modem output (non-stripping) and print as verbose
static int read_response_verbose(char* buf, int maxlen, int timeout_ms, const char* tag) {
  int idx = 0;
  unsigned long start = millis();
  while (millis() - start < (unsigned long)timeout_ms && idx < maxlen-1) {
    while (SimSerial.available() && idx < maxlen-1) {
      char c = SimSerial.read();
      buf[idx++] = c;
    }
    // if we saw newline end, break to allow per-line logging
    if (idx > 0 && buf[idx-1] == '\n') break;
    delay(5);
  }
  buf[idx] = 0;
  if (idx > 0) trace_modem_resp(tag, buf);
  return idx;
}

// non-verbose wrapper kept for internal use
static int read_response(char* buf, int maxlen, int timeout_ms) {
  return read_response_verbose(buf, maxlen, timeout_ms, "RESP");
}

// wait for a token in modem stream, printing whatever we read
static bool wait_for_token_verbose(const char* token, int timeout_ms) {
  char buf[1024];
  unsigned long start = millis();
  while (millis() - start < (unsigned long)timeout_ms) {
    int n = read_response_verbose(buf, sizeof(buf), 500, "CHECK");
    if (n > 0 && strstr(buf, token)) return true;
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  return false;
}

static int at_cmd_expect_ok_verbose(const char* cmd, int timeout_ms) {
  at_send(cmd);
  char buf[1024];
  unsigned long start = millis();
  while (millis() - start < (unsigned long)timeout_ms) {
    int n = read_response_verbose(buf, sizeof(buf), 500, "OK?");
    if (n <= 0) { vTaskDelay(50 / portTICK_PERIOD_MS); continue; }
    if (strstr(buf, "OK")) return 1;
    if (strstr(buf, "ERROR")) return 0;
  }
  return 0;
}

// ----- network / SIM helpers ------------------------------------------------

static int check_network_registration_verbose() {
  at_send("AT+CREG?");
  char buf[256];
  int n = read_response_verbose(buf, sizeof(buf), 500, "CREG");
  if (n > 0) {
    int m=-1, stat=-1;
    if (sscanf(buf, "+CREG: %d,%d", &m, &stat) >= 2) {
      Serial.printf("[NET] CREG stat=%d\r\n", stat);
      if (stat == 1 || stat == 5) return 1;
    } else if (strstr(buf, "0,1") || strstr(buf, "0,5")) return 1;
  }
  // fallback to CGATT?
  at_send("AT+CGATT?");
  n = read_response_verbose(buf, sizeof(buf), 500, "CGATT");
  if (n > 0 && strstr(buf, "+CGATT: 1")) return 1;
  return 0;
}

static int sim_unlock_if_needed_verbose() {
  const char* pin = cfg_get_sim_pin();
  if (!pin || strlen(pin) == 0) return 1;
  at_send("AT+CPIN?");
  char buf[256];
  int n = read_response_verbose(buf, sizeof(buf), 1000, "CPIN");
  // if response doesn't indicate SIM PIN required, continue
  if (n > 0 && strstr(buf, "SIM PIN") == NULL && strstr(buf, "+CPIN: \"SIM PIN\"") == NULL) return 1;
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "AT+CPIN=\"%s\"", pin);
  return at_cmd_expect_ok_verbose(cmd, 5000);
}

static int setup_gprs_bearer_once_verbose() {
  char buf[512];
  char cmd[256];
  const char* apn = cfg_get_apn();
  const char* user = cfg_get_apn_user();
  const char* pass = cfg_get_apn_pass();

  Serial.printf("[GPRS] Setting APN='%s' user='%s' pass='%s'\r\n", apn ? apn : "", user ? user : "", pass ? pass : "");

  snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
  at_send(cmd);
  read_response_verbose(buf, sizeof(buf), 1000, "CGDCONT");

  at_send("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  read_response_verbose(buf, sizeof(buf), 1000, "SAPBR");

  snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
  at_send(cmd); read_response_verbose(buf, sizeof(buf), 1000, "SAPBR");

  if (user && user[0]) {
    snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"USER\",\"%s\"", user);
    at_send(cmd); read_response_verbose(buf, sizeof(buf), 1000, "SAPBR");
  }
  if (pass && pass[0]) {
    snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"PWD\",\"%s\"", pass);
    at_send(cmd); read_response_verbose(buf, sizeof(buf), 1000, "SAPBR");
  }

  at_send("AT+SAPBR=1,1");
  if (!wait_for_token_verbose("OK", 15000)) {
    // retry once
    at_send("AT+SAPBR=0,1"); read_response_verbose(buf, sizeof(buf), 1000, "SAPBR");
    at_send("AT+SAPBR=1,1");
    if (!wait_for_token_verbose("OK", 15000)) {
      Serial.println("[GPRS] SAPBR open failed");
      return 0;
    }
  }

  at_send("AT+SAPBR=2,1");
  if (!wait_for_token_verbose("+SAPBR:", 3000)) {
    Serial.println("[GPRS] SAPBR query failed");
    return 0;
  }
  Serial.println("[GPRS] bearer opened OK");
  return 1;
}

int sim800_force_gprs_setup(void) {
  if (millis() - last_gprs_attempt_ms < 3000) return gprs_ready ? 1 : 0;
  last_gprs_attempt_ms = millis();
  xSemaphoreTake(gprs_mutex, portMAX_DELAY);
  if (gprs_ready) { xSemaphoreGive(gprs_mutex); return 1; }

  if (!sim_unlock_if_needed_verbose()) {
    Serial.println("[SIM] unlock failed");
    xSemaphoreGive(gprs_mutex);
    return 0;
  }

  Serial.println("[SIM] waiting for network registration (up to 120s)...");
  unsigned long start = millis();
  bool registered = false;
  while (millis() - start < 120000) {
    if (check_network_registration_verbose()) { registered = true; break; }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  if (!registered) {
    Serial.println("[SIM] network registration failed");
    xSemaphoreGive(gprs_mutex);
    return 0;
  }
  Serial.println("[SIM] network registered");

  if (!setup_gprs_bearer_once_verbose()) {
    Serial.println("[SIM] bearer setup failed");
    xSemaphoreGive(gprs_mutex);
    return 0;
  }

  gprs_ready = true;
  xSemaphoreGive(gprs_mutex);
  Serial.println("[SIM] GPRS ready");
  return 1;
}

// ----- HTTP POST with verbose logging -------------------------------------

static int http_post_sync_with_setup_verbose(const char *url, const char *json, const char *hdr_key, const char *hdr_val) {
  if (!gprs_ready) {
    if (!sim800_force_gprs_setup()) {
      Serial.println("[HTTP] unable to ensure GPRS before POST");
      return 0;
    }
  }

  char buf[2048];
  at_send("AT+HTTPTERM");
  read_response_verbose(buf, sizeof(buf), 500, "HTTPTERM");

  at_send("AT+HTTPINIT");
  read_response_verbose(buf, sizeof(buf), 2000, "HTTPINIT");

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url);
  at_send(cmd); read_response_verbose(buf, sizeof(buf), 2000, "HTTPPARA");

  at_send("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  read_response_verbose(buf, sizeof(buf), 1000, "HTTPPARA");

  if (hdr_key && hdr_val && hdr_key[0] && hdr_val[0]) {
    snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"USERDATA\",\"%s: %s\"", hdr_key, hdr_val);
    at_send(cmd); read_response_verbose(buf, sizeof(buf), 1000, "HTTPPARA");
  }

  int len = (int)strlen(json);
  snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%d,10000", len);
  at_send(cmd);
  read_response_verbose(buf, sizeof(buf), 5000, "HTTPDATA");

  // send payload
  trace_at_cmd("[HTTP BODY]");
  SimSerial.print(json);
  read_response_verbose(buf, sizeof(buf), 5000, "HTTPWRITE");

  at_send("AT+HTTPACTION=1");
  unsigned long start = millis();
  while (millis() - start < 30000) {
    int n = read_response_verbose(buf, sizeof(buf), 1000, "HTTPACTION");
    if (n > 0 && strstr(buf, "+HTTPACTION:")) {
      int method, status, datalen;
      if (sscanf(buf, "+HTTPACTION: %d,%d,%d", &method, &status, &datalen) >= 3) {
        Serial.printf("[HTTP] action status=%d len=%d\r\n", status, datalen);
        if (status >= 200 && status < 300) {
          at_send("AT+HTTPREAD");
          read_response_verbose(buf, sizeof(buf), 2000, "HTTPREAD");
          at_send("AT+HTTPTERM");
          return 1;
        } else {
          at_send("AT+HTTPTERM");
          return 0;
        }
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  at_send("AT+HTTPTERM");
  return 0;
}

int sim800_http_post_block(const char* url, const char* json, const char* header_key, const char* header_value) {
  return http_post_sync_with_setup_verbose(url, json, header_key, header_value);
}

int sim800_http_post_async(const char* url, const char* json, const char* header_key, const char* header_value) {
  if (!simQ) return 0;
  size_t len = strlen(json) + strlen(url) + 256;
  char* buf = (char*)malloc(len);
  if (!buf) return 0;
  snprintf(buf, len, "POST|%s|%s|%s|%s", url, header_key?header_key:"", header_value?header_value:"", json);
  if (xQueueSend(simQ, &buf, 0) != pdTRUE) { free(buf); return 0; }
  return 1;
}

void sim800_task(void *pvParameters) {
  (void)pvParameters;
  char* item = NULL;
  for (;;) {
    if (xQueueReceive(simQ, &item, portMAX_DELAY) == pdTRUE) {
      char* p = item;
      char* parts[5] = {0};
      int idx = 0;
      parts[idx++] = strtok(p, "|");
      while (idx < 5 && parts[idx-1]) parts[idx++] = strtok(NULL, "|");
      if (parts[0] && strcmp(parts[0], "POST") == 0 && parts[4]) {
        const char* url = parts[1] ? parts[1] : "";
        const char* hk  = parts[2] ? parts[2] : "";
        const char* hv  = parts[3] ? parts[3] : "";
        const char* body = parts[4];

        if (!gprs_ready) {
          int ok = sim800_force_gprs_setup();
          if (!ok) {
            Serial.println("[SIM Task] GPRS setup failed; writing to SD");
            sd_enqueue_eam(body);
            free(item);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
          }
        }

        int ok = http_post_sync_with_setup_verbose(url, body, hk && hk[0] ? hk : NULL, hv && hv[0] ? hv : NULL);
        if (!ok) {
          gprs_ready = false;
          sd_enqueue_eam(body);
        }
      }
      free(item);
    }
  }
}
