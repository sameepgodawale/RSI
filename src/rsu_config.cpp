#include "rsu_config.h"
#include <Preferences.h>
#include "include/cms_config.h"

// NVS namespace
static Preferences prefs;
static const char* PREF_NS = "rsu_cfg";

// cache pointers (owned by Preferences API; we copy when returning)
static String s_rsu_api_key;
static String s_cms_url;
static String s_apn;
static String s_apn_user;
static String s_apn_pass;
static String s_sim_pin;

void config_init() {
  prefs.begin(PREF_NS, false); // RW
  s_rsu_api_key = prefs.getString("rsu_api_key", "");
  s_cms_url     = prefs.getString("cms_url", "");
  s_apn         = prefs.getString("apn", "");
  s_apn_user    = prefs.getString("apn_user", "");
  s_apn_pass    = prefs.getString("apn_pass", "");
  s_sim_pin     = prefs.getString("sim_pin", "");
  // If not set, use compile-time fallbacks for an immediate debug print
  if (s_rsu_api_key.length() == 0) s_rsu_api_key = RSU_API_KEY;
  if (s_cms_url.length() == 0)     s_cms_url     = CMS_API_URL;
  if (s_apn.length() == 0)         s_apn         = APN_NAME;
  if (s_apn_user.length() == 0)    s_apn_user    = APN_USER;
  if (s_apn_pass.length() == 0)    s_apn_pass    = APN_PASS;
  if (s_sim_pin.length() == 0)     s_sim_pin     = SIM_PIN;
}

const char* cfg_get_rsu_api_key() { return s_rsu_api_key.c_str(); }
const char* cfg_get_cms_url()     { return s_cms_url.c_str(); }
const char* cfg_get_apn()         { return s_apn.c_str(); }
const char* cfg_get_apn_user()    { return s_apn_user.c_str(); }
const char* cfg_get_apn_pass()    { return s_apn_pass.c_str(); }
const char* cfg_get_sim_pin()     { return s_sim_pin.c_str(); }

bool cfg_set_rsu_api_key(const char* v) {
  prefs.putString("rsu_api_key", v ? v : "");
  s_rsu_api_key = prefs.getString("rsu_api_key", RSU_API_KEY);
  return true;
}
bool cfg_set_cms_url(const char* v) {
  prefs.putString("cms_url", v ? v : "");
  s_cms_url = prefs.getString("cms_url", CMS_API_URL);
  return true;
}
bool cfg_set_apn(const char* v) {
  prefs.putString("apn", v ? v : "");
  s_apn = prefs.getString("apn", APN_NAME);
  return true;
}
bool cfg_set_apn_user(const char* v) {
  prefs.putString("apn_user", v ? v : "");
  s_apn_user = prefs.getString("apn_user", APN_USER);
  return true;
}
bool cfg_set_apn_pass(const char* v) {
  prefs.putString("apn_pass", v ? v : "");
  s_apn_pass = prefs.getString("apn_pass", APN_PASS);
  return true;
}
bool cfg_set_sim_pin(const char* v) {
  prefs.putString("sim_pin", v ? v : "");
  s_sim_pin = prefs.getString("sim_pin", SIM_PIN);
  return true;
}
