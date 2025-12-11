#include "web_config.h"
#include <WiFi.h>
#include <WebServer.h>
#include "rsu_config.h"

static WebServer server(80);
static const char* AP_SSID = "RSU-Setup";
static const char* AP_PASS = "rsu12345";

static String page_form() {
  String apn = cfg_get_apn();
  String apn_user = cfg_get_apn_user();
  String apn_pass = cfg_get_apn_pass();
  String sim_pin = cfg_get_sim_pin();
  String api_key = cfg_get_rsu_api_key();
  String cms_url = cfg_get_cms_url();
  String s = "<!doctype html><html><head><meta charset='utf-8'><title>RSU Config</title></head><body>";
  s += "<h2>RSU Configuration</h2>";
  s += "<form method='POST' action='/save'>";
  s += "APN: <input name='apn' value='" + apn + "' size='40'><br>";
  s += "APN User: <input name='apn_user' value='" + apn_user + "' size='40'><br>";
  s += "APN Pass: <input name='apn_pass' value='" + apn_pass + "' size='40'><br>";
  s += "SIM PIN: <input name='sim_pin' value='" + sim_pin + "' size='20'><br>";
  s += "RSU API Key: <input name='rsu_api_key' value='" + api_key + "' size='64'><br>";
  s += "CMS URL: <input name='cms_url' value='" + cms_url + "' size='80'><br>";
  s += "<input type='submit' value='Save'>";
  s += "</form>";
  s += "<p><a href='/reboot'>Save & Reboot</a> (saves and reboots the device)</p>";
  s += "</body></html>";
  return s;
}

static void handle_root() {
  server.send(200, "text/html", page_form());
}

static void handle_save() {
  // read params and persist
  if (server.hasArg("apn")) cfg_set_apn(server.arg("apn").c_str());
  if (server.hasArg("apn_user")) cfg_set_apn_user(server.arg("apn_user").c_str());
  if (server.hasArg("apn_pass")) cfg_set_apn_pass(server.arg("apn_pass").c_str());
  if (server.hasArg("sim_pin")) cfg_set_sim_pin(server.arg("sim_pin").c_str());
  if (server.hasArg("rsu_api_key")) cfg_set_rsu_api_key(server.arg("rsu_api_key").c_str());
  if (server.hasArg("cms_url")) cfg_set_cms_url(server.arg("cms_url").c_str());
  server.send(200, "text/html", "<html><body><h3>Saved. Reboot required to apply fully. <a href='/'>Back</a></h3></body></html>");
}

static void handle_reboot() {
  server.send(200, "text/html", "<html><body><h3>Rebooting...</h3></body></html>");
  delay(500);
  ESP.restart();
}

void web_config_start() {
  // start AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[WEBCFG] AP started: %s, IP: %s\r\n", AP_SSID, apIP.toString().c_str());

  server.on("/", handle_root);
  server.on("/save", HTTP_POST, handle_save);
  server.on("/reboot", HTTP_GET, handle_reboot);
  server.begin();
  Serial.println("[WEBCFG] Web server started on port 80");
}

void web_config_stop() {
  server.stop();
  WiFi.softAPdisconnect(true);
}
