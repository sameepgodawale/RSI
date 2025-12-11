#include "sd_store.h"
#include <SD.h>

static int sd_cs = -1;

bool sd_init(int cs_pin) {
  sd_cs = cs_pin;
  if (!SD.begin(sd_cs)) {
    return false;
  }
  return true;
}

int sd_enqueue_eam(const char* json) {
  if (sd_cs < 0) return 0;
  File f = SD.open("/queue_eam.ndjson", FILE_APPEND);
  if (!f) return 0;
  f.println(json);
  f.close();
  return 1;
}
