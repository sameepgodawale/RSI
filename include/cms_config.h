#ifndef CMS_CONFIG_H
#define CMS_CONFIG_H

// Compile-time defaults (fallback if runtime config not set)
#ifndef RSU_API_KEY
#define RSU_API_KEY "REPLACE_WITH_KEY"
#endif

#ifndef CMS_API_URL
#define CMS_API_URL "http://cms.example.com/api/v1/incidents/report"
#endif

#ifndef APN_NAME
#define APN_NAME "your_apn_here"
#endif

#ifndef APN_USER
#define APN_USER ""
#endif

#ifndef APN_PASS
#define APN_PASS ""
#endif

#ifndef SIM_PIN
#define SIM_PIN ""
#endif

// runtime accessors are provided by rsu_config module
const char* rsu_api_key_fallback(void);
const char* cms_api_url_fallback(void);
const char* apn_name_fallback(void);
const char* apn_user_fallback(void);
const char* apn_pass_fallback(void);
const char* sim_pin_fallback(void);

#endif
