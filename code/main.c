#include <stddef.h>

#include "ctrl/scbi.h"
#include "ctrl/com/mqtt.h"
#include "tool/logger.h"




int main(void)
{
  struct mqtt_handle * mqtt;
  struct scbi_handle * scbi;

//  log_set_level(LL_CRITICAL, TRUE);
  log_set_level(LL_INFO, TRUE);
  log_set_level(LL_DEBUG, TRUE);

  mqtt = mqtt_init("MTDC");
  if (mqtt)
  {
    scbi = scbi_init("can0", mqtt);
    if (scbi)
    {
      scbi_register_dlg_sensor(scbi, DST_UNDEFINED, 0, "collector");
      scbi_register_dlg_sensor(scbi, DST_UNDEFINED, 1, "storage");

      scbi_register_dlg_relay(scbi, DRM_RELAYMODE_SWITCHED, DRE_UNSELECTED, 0, "pump_state");
      scbi_register_dlg_relay(scbi, DRM_RELAYMODE_PWM     , DRE_UNSELECTED, 0, "pump");
      scbi_register_dlg_relay(scbi, DRM_RELAYMODE_SWITCHED, DRE_UNSELECTED, 1, "pump1_state");
      scbi_register_dlg_relay(scbi, DRM_RELAYMODE_PWM     , DRE_UNSELECTED, 1, "pump1");

      scbi_register_dlg_overview(scbi, DOT_DAYS, DOM_00, "days0");
      scbi_register_dlg_overview(scbi, DOT_DAYS, DOM_01, "days1");
      scbi_register_dlg_overview(scbi, DOT_DAYS, DOM_02, "days2");
      scbi_register_dlg_overview(scbi, DOT_WEEKS, DOM_00, "weeks0");
      scbi_register_dlg_overview(scbi, DOT_WEEKS, DOM_01, "weeks1");
      scbi_register_dlg_overview(scbi, DOT_WEEKS, DOM_02, "weeks2");
      scbi_register_dlg_overview(scbi, DOT_MONTHS, DOM_00, "months0");
      scbi_register_dlg_overview(scbi, DOT_MONTHS, DOM_01, "months1");
      scbi_register_dlg_overview(scbi, DOT_MONTHS, DOM_02, "months2");
      scbi_register_dlg_overview(scbi, DOT_YEARS, DOM_00, "years0");
      scbi_register_dlg_overview(scbi, DOT_YEARS, DOM_01, "years1");
      scbi_register_dlg_overview(scbi, DOT_YEARS, DOM_02, "years2");
      scbi_register_dlg_overview(scbi, DOT_TOTAL, DOM_00, "total0");
      scbi_register_dlg_overview(scbi, DOT_TOTAL, DOM_01, "total1");
      scbi_register_dlg_overview(scbi, DOT_TOTAL, DOM_02, "total2");
      scbi_register_dlg_overview(scbi, DOT_STATUS, DOM_00, "status0");
      scbi_register_dlg_overview(scbi, DOT_STATUS, DOM_01, "status1");
      scbi_register_dlg_overview(scbi, DOT_STATUS, DOM_02, "status2");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN07, DOM_00, "unknown070");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN07, DOM_01, "unknown071");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN07, DOM_02, "unknown072");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN08, DOM_00, "unknown080");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN08, DOM_01, "unknown081");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN08, DOM_02, "unknown082");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN09, DOM_00, "unknown090");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN09, DOM_01, "unknown091");
      scbi_register_dlg_overview(scbi, DOT_UNKNOWN09, DOM_02, "unknown092");

      scbi_update(scbi);
      scbi_close(scbi);
    }
    mqtt_close(mqtt);
  }
	return 0;
}
