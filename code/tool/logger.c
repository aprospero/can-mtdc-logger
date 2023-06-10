#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "logger.h"
#include "stringhelp.h"

typedef void (*logfct)(const enum log_level ll, const char * format, va_list argp);

static struct logger_state
{
  size_t        level[LL_COUNT];
  logfct        fct;
} log;

const char * log_level_txt[] = {
   "NONE",
   "CRIT ",
   "ERROR",
   "WARN ",
   "INFO ",
   "EVENT",
   "DEBUG",
   "DBGMR",
   "DBGMX"
};

const char * log_level_fulltxt[]  = {
    "NONE",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "INFO",
    "EVENT",
    "DEBUG",
    "DEBUG_MORE",
    "DEBUG_MAX"
 };

const char * log_facility_txt [] = {
  "stdout" ,
  "user"   ,
  "local0" ,
  "local1" ,
  "local2" ,
  "local3" ,
  "local4" ,
  "local5" ,
  "local6" ,
  "local7"
};

static void log_stdout_stderr(const enum log_level ll, const char * format, va_list ap)
{
  static char tmp[MAX_LOG_LEN];
  static char tim[64];

  FILE * fd;
  struct tm * tm;
  struct timespec tp;

  clock_gettime(CLOCK_REALTIME, &tp);
  tm = localtime(&tp.tv_sec);
  strftime(tim, sizeof(tim), "%d.%m.%y %H:%M:%S",tm);

  vsprintf(tmp, format, ap);

  tmp[sizeof(tmp) - 1] = '\0';

  switch (ll)
  {
    case LL_CRITICAL:
    case LL_ERROR   :
    case LL_WARN    : fd = stderr; break;
    default         : fd = stdout; break;
  }
  fprintf (fd, "[%s.%ld][%s] %s\n", tim, tp.tv_nsec / 1000, log_level_txt[ll], tmp);
}



const int ll_translation[LL_COUNT] =
{
  LOG_CRIT,    /*  LL_CRITICAL    */
  LOG_ERR,     /*  LL_ERROR       */
  LOG_WARNING, /*  LL_WARN        */
  LOG_NOTICE,  /*  LL_INFO        */
  LOG_INFO,    /*  LL_EVENT       */
  LOG_DEBUG,   /*  LL_DEBUG       */
  LOG_DEBUG,   /*  LL_DEBUG_MORE  */
  LOG_DEBUG    /*  LL_DEBUG_MAX   */
};

static void log_syslog(const enum log_level ll, const char * format, va_list ap)
{
  vsyslog(ll_translation[ll], format, ap);
}

const int lf_translation[LF_COUNT] =
{
  LOG_USER	  , /*  LF_USER     random user-level messages */
  LOG_LOCAL0  , /*  LF_LOCAL0   reserved for local use */
  LOG_LOCAL1  , /*  LF_LOCAL1   reserved for local use */
  LOG_LOCAL2  , /*  LF_LOCAL2   reserved for local use */
  LOG_LOCAL3  , /*  LF_LOCAL3   reserved for local use */
  LOG_LOCAL4  , /*  LF_LOCAL4   reserved for local use */
  LOG_LOCAL5  , /*  LF_LOCAL5   reserved for local use */
  LOG_LOCAL6  , /*  LF_LOCAL6   reserved for local use */
  LOG_LOCAL7  , /*  LF_LOCAL7   reserved for local use */
};


void log_init(const char * ident, enum log_facility facility, enum log_level default_ll)
{
  memset(&log,0,sizeof(log));
  log_set_level_state(default_ll, TRUE);

  if (facility > LF_STDOUT && facility < LF_COUNT && ident)
  {
    openlog(ident, 0, lf_translation[facility - 1]);
    log.fct = log_syslog;
  }
  else
  {
    if (facility != LF_STDOUT)
    {
      const char * fac_name = facility >= LF_COUNT ? "invalid" : log_facility_txt[facility];
      const char * ident_name = ident == NULL ? "unknown" : ident;
      log_push(LL_WARN, "Logging via STDOUT/STDERR. %s facility and %s ident not feasible.", fac_name, ident_name);
    }
    log.fct = log_stdout_stderr;
  }
}

void log_set_level_state(enum log_level ll, size_t active)
{
  if (ll < LL_NONE)
    return;
  if (active)
  {
    if (ll >= ARRLEN(log.level))
      ll = (enum log_level) (LL_COUNT - 1);
    do {
      log.level[ll] = TRUE;
    } while (ll-- != LL_NONE);
  }
  else if (ll < ARRLEN(log.level))
    log.level[ll] = FALSE;
}

int log_get_level_state(enum log_level ll)
{
  return ll < 0 || ll >= ARRLEN(log.level) ? FALSE : log.level[ll];
}

const char * log_get_level_name(enum log_level ll, int do_fulltext)
{
  if (ll > 0 && ll < ARRLEN(log_level_fulltxt))
    return do_fulltext ? log_level_fulltxt[ll] : log_level_txt[ll];
  return NULL;
}

enum log_level log_get_level_no(const char * level)
{
  for (int i = 0; i < ARRLEN(log_level_fulltxt); i++)
  {
    if (stricmp(log_level_fulltxt[i], level) == 0)
      return (enum log_level) i;
  }
  return LL_NONE;
}

enum log_facility log_get_facility(const char * facility)
{
  for (int i = 0; i < ARRLEN(log_facility_txt); i++)
  {
    if (stricmp(log_facility_txt[i], facility) == 0)
      return (enum log_facility) i;
  }
  return LF_COUNT;
}

const char * log_get_facility_name(enum log_facility lf)
{
  if  (lf > 0 && lf < ARRLEN(log_facility_txt))
    return log_facility_txt[lf];
  return NULL;
}


void log_push(const enum log_level ll, const char * format, ...)
{
  if (ll >= 0 && ll < LL_COUNT && log.level[ll] && log.fct)
  {
    va_list ap;
    va_start(ap, format);
    log.fct(ll, format, ap);
    va_end(ap);
  }
}
