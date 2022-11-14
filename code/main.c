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
      scbi_update(scbi);
      scbi_close(scbi);
    }
    mqtt_close(mqtt);
  }
	return 0;
}
