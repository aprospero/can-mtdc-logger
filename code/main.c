#include <stddef.h>

#include "ctrl/scbi.h"
#include "tool/logger.h"




int main(void)
{
	struct scbi_handle * scbi;

//  log_set_level(LL_CRITICAL, TRUE);
  log_set_level(LL_INFO, TRUE);
  log_set_level(LL_DEBUG, TRUE);

  scbi = scbi_open("can0");

  scbi_update(scbi);
  scbi_close(scbi);
  return 0;
}
