#include "sd_retry.h"
#include "sd_store.h"
#include "sim800.h"
#include <SD.h>
#include <vector>

void sd_retry_task(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    if (!SD.begin()) { vTaskDelay(5000 / portTICK_PERIOD_MS); continue; }
    File f = SD.open("/queue_eam.ndjson", FILE_READ);
    if (!f) { vTaskDelay(5000 / portTICK_PERIOD_MS); continue; }
    // Read file into memory
    String all = "";
    while (f.available()) { all += (char)f.read(); }
    f.close();
    // parse lines and try sync upload
    std::vector<String> survivors;
    int start = 0;
    while (start < all.length()) {
      int nl = all.indexOf('\n', start);
      if (nl < 0) nl = all.length();
      String line = all.substring(start, nl);
      start = nl + 1;
      if (line.length() == 0) continue;
      // attempt sync post
      if (!sim800_http_post_block(CMS_API_URL, line.c_str(), "x-api-key", cfg_get_rsu_api_key())) {
        survivors.push_back(line);
      }
    }
    // rewrite file with survivors
    SD.remove("/queue_eam.ndjson");
    if (!survivors.empty()) {
      File fw = SD.open("/queue_eam.ndjson", FILE_WRITE);
      for (auto &s: survivors) { fw.println(s); }
      fw.close();
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
