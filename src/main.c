#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "ctrl/scbi_glue.h"
#include "ctrl/com/mqtt.h"
#include "ctrl/logger.h"
#include "args.h"
#include "version.h"


void clean_exit_on_sig(int sig_num)
{
  LG_CRITICAL("Signal %d received", sig_num);
  exit(0);
}

int main(int argc, char * argv[])
{
  struct cansorella_config config;
  struct mqtt_handle * mqtt;
  struct scbi_glue_handle * scbi_glue;
  struct scbi_handle * scbi;

  parseArgs(argc, argv, &config);

  log_init(config.prg_name, config.log_facility, config.log_level);

  log_push(LL_NONE, "##########################################################################");
  log_push(LL_NONE, "Starting %s "APP_VERSION" - on:%s, LogFacility:%s Level:%s.",
                       config.prg_name, config.can_device, log_get_facility_name(config.log_facility), log_get_level_name(config.log_level, TRUE));
  log_push(LL_NONE, "##########################################################################");

  signal(SIGABRT, clean_exit_on_sig);
  signal(SIGINT,  clean_exit_on_sig);
  signal(SIGSEGV, clean_exit_on_sig);
  signal(SIGTERM, clean_exit_on_sig);
  signal(SIGPIPE, SIG_IGN);

  mqtt = mqtt_init(&config.mqtt);
  if (mqtt)
  {
    scbi = scbi_init(malloc, scbi_glue_log, 300);
    scbi_glue = scbi_glue_init(scbi, config.can_device, mqtt);
    if (scbi)
    {
      scbi_register_sensor(scbi, 0, DST_UNDEFINED, "collector");
      scbi_register_sensor(scbi, 1, DST_UNDEFINED, "storage");

      scbi_register_relay(scbi, 0, DRM_RELAYMODE_SWITCHED, DRE_UNSELECTED, "pump_on");
      scbi_register_relay(scbi, 2, DRM_RELAYMODE_PWM     , DRE_UNSELECTED, "pump");
      scbi_register_relay(scbi, 1, DRM_RELAYMODE_SWITCHED, DRE_UNSELECTED, "relay1");

      scbi_register_overview(scbi, DOT_DAYS, DOM_00, "days0");
      scbi_register_overview(scbi, DOT_DAYS, DOM_01, "days1");
      scbi_register_overview(scbi, DOT_DAYS, DOM_02, "days2");
      scbi_register_overview(scbi, DOT_WEEKS, DOM_00, "weeks0");
      scbi_register_overview(scbi, DOT_WEEKS, DOM_01, "weeks1");
      scbi_register_overview(scbi, DOT_WEEKS, DOM_02, "weeks2");
      scbi_register_overview(scbi, DOT_MONTHS, DOM_00, "months0");
      scbi_register_overview(scbi, DOT_MONTHS, DOM_01, "months1");
      scbi_register_overview(scbi, DOT_MONTHS, DOM_02, "months2");
      scbi_register_overview(scbi, DOT_YEARS, DOM_00, "years0");
      scbi_register_overview(scbi, DOT_YEARS, DOM_01, "years1");
      scbi_register_overview(scbi, DOT_YEARS, DOM_02, "years2");
      scbi_register_overview(scbi, DOT_TOTAL, DOM_00, "total0");
      scbi_register_overview(scbi, DOT_TOTAL, DOM_01, "total1");
      scbi_register_overview(scbi, DOT_TOTAL, DOM_02, "total2");
      scbi_register_overview(scbi, DOT_STATUS, DOM_00, "status0");
      scbi_register_overview(scbi, DOT_STATUS, DOM_01, "status1");
      scbi_register_overview(scbi, DOT_STATUS, DOM_02, "status2");
      scbi_register_overview(scbi, DOT_UNKNOWN07, DOM_00, "unknown070");
      scbi_register_overview(scbi, DOT_UNKNOWN07, DOM_01, "unknown071");
      scbi_register_overview(scbi, DOT_UNKNOWN07, DOM_02, "unknown072");
      scbi_register_overview(scbi, DOT_UNKNOWN08, DOM_00, "unknown080");
      scbi_register_overview(scbi, DOT_UNKNOWN08, DOM_01, "unknown081");
      scbi_register_overview(scbi, DOT_UNKNOWN08, DOM_02, "unknown082");
      scbi_register_overview(scbi, DOT_UNKNOWN09, DOM_00, "unknown090");
      scbi_register_overview(scbi, DOT_UNKNOWN09, DOM_01, "unknown091");
      scbi_register_overview(scbi, DOT_UNKNOWN09, DOM_02, "unknown092");

      scbi_glue_update(scbi_glue);
      scbi_glue_close(scbi_glue);
    }
    mqtt_close(mqtt);
  }
	return 0;
}
