#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "logger.h"

typedef void (*logfct)(const enum log_level ll, const char * format, va_list argp);

static struct logger_state
{
  enum log_mode mode;
  size_t        level[LL_COUNT];
  logfct        fct;
} log;

const char * level_txt[] = {
   "CRIT ",
   "ERROR",
   "WARN ",
   "INFO ",
   "EVENT",
   "DEBUG",
   "DBGMR",
   "DBGMX"
};

static void log_stdout_stderr(const enum log_level ll, const char * format, va_list ap)
{
  static char tmp[MAX_LOG_LEN];

  FILE * fd;
  time_t ts = time(NULL);
  char * tim = asctime(localtime(&ts));
  size_t len = strlen(tim);
  if (len)
    tim[len - 1] = '\0';

  vsprintf(tmp, format, ap);

  tmp[sizeof(tmp) - 1] = '\0';

  switch (ll)
  {
    case LL_CRITICAL:
    case LL_ERROR   :
    case LL_WARN    : fd = stderr; break;
    default         : fd = stdout; break;
  }
  fprintf (fd, "[%s][%s] %s\n", tim, level_txt[ll], tmp);
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
  LOG_USER    , /*  LF_NONE     default is user logs */
  LOG_KERN	  , /*  LF_KERN     kernel messages */
  LOG_USER	  , /*  LF_USER     random user-level messages */
  LOG_MAIL	  , /*  LF_MAIL     mail system */
  LOG_DAEMON  , /*  LF_DAEMON   system daemons */
  LOG_AUTH	  , /*  LF_AUTH     security/authorization messages */
  LOG_SYSLOG  , /*  LF_SYSLOG   messages generated internally by syslogd */
  LOG_LPR		  , /*  LF_LPR      line printer subsystem */
  LOG_NEWS	  , /*  LF_NEWS     network news subsystem */
  LOG_UUCP	  , /*  LF_UUCP     UUCP subsystem */
  LOG_CRON	  , /*  LF_CRON     clock daemon */
  LOG_AUTHPRIV, /*  LF_AUTHPRIV security/authorization messages (private) */
  LOG_FTP		  , /*  LF_FTP      ftp daemon */
  LOG_LOCAL0	, /*  LF_LOCAL0   reserved for local use */
  LOG_LOCAL1	, /*  LF_LOCAL1   reserved for local use */
  LOG_LOCAL2	, /*  LF_LOCAL2   reserved for local use */
  LOG_LOCAL3	, /*  LF_LOCAL3   reserved for local use */
  LOG_LOCAL4	, /*  LF_LOCAL4   reserved for local use */
  LOG_LOCAL5	, /*  LF_LOCAL5   reserved for local use */
  LOG_LOCAL6	, /*  LF_LOCAL6   reserved for local use */
  LOG_LOCAL7	, /*  LF_LOCAL7   reserved for local use */
};


void log_init(enum log_mode mode, const char * ident, enum log_facility facility)
{
  memset(&log,0,sizeof(log));
  log.mode = mode;
  switch (mode)
  {
    case LM_STDOUT_STDERR: log.fct = log_stdout_stderr; break;
    case LM_SYSLOG       : log.fct = log_syslog;
      if (facility >= 0 && facility < LF_COUNT && ident)
      {
        openlog(ident, 0, lf_translation[facility]);
      }
      else
      {
        log.mode = LM_STDOUT_STDERR;
        log.fct = log_stdout_stderr;
      }
      break;
    default: log.fct = NULL; break;
  }
}

void log_set_level(enum log_level ll, size_t active)
{
  if (active)
  {
    log.level[ll] = active;
    for (++ll; ll < LL_DEBUG; ll++)
      log.level[ll] = active;
  }
  else log.level[ll] = FALSE;
}

int log_get_level(enum log_level ll)
{
  return log.level[ll];
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
