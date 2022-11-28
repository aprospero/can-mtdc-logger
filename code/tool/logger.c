#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

static size_t level[LL_COUNT] = { 0 };

const char * level_txt[] = {
   "CRIT ",
   "ERROR",
   "WARN ",
   "INFO ",
   "EVENT",
   "DEBUG",
   "DEBUG_MORE",
   "DEBUG_MAX"
};

void log_set_level(enum log_level ll, size_t active)
{
  if (active)
  {
    level[ll] = active;
    for (++ll; ll < LL_DEBUG; ll++)
      level[ll] = active;
  }
  else level[ll] = FALSE;
}

int log_get_level(enum log_level ll)
{
  return level[ll];
}


void log_push(const enum log_level ll, const char * format, ...)
{
  static char tmp[MAX_LOG_LEN];

  if (level[ll])
  {
    FILE * fd;
    time_t ts = time(NULL);
//    char * tim = asctime(localtime(&ts));
//    size_t len = strlen(tim);
//    if (len)
//      tim[len - 1] = '\0';

    va_list ap;
    va_start(ap, format);
    vsnprintf(tmp, sizeof(tmp), format, ap);
    va_end(ap);

    tmp[sizeof(tmp) - 1] = '\0';

    switch (ll)
    {
      case LL_CRITICAL:
      case LL_ERROR   :
      case LL_WARN    : fd = stderr; break;
      default         : fd = stdout; break;
    }
    fprintf (fd, "[%u][%s] %s\n", ts, level_txt[ll], tmp);
  }
}
