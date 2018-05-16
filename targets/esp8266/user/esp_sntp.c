#include "esp_sntp.h"

static char *servers[] =
{
  "0.pool.ntp.org",
  "1.pool.ntp.org",
  "2.pool.ntp.org",
  "3.pool.ntp.org"
};


/**
 * Initialize the RTC clock with the current timestamp
 */
void init_esp_sntp (void)
{
  if (sdk_wifi_station_get_connect_status () == STATION_GOT_IP && !sntp_been_initialized)
  {
    sntp_set_update_delay (10 * 60000);
    sntp_initialize (&tz);
    sntp_set_servers (servers, sizeof (servers) / sizeof (char *));
    sntp_been_initialized = true;
    vTaskDelay (1000 / portTICK_PERIOD_MS);
  }
} /* init_esp_sntp */
