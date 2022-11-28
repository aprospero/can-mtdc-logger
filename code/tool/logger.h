#ifndef _TOOL_LOGGER__H
#define _TOOL_LOGGER__H

#include <stdarg.h>

enum log_mode
{
  LM_STDOUT_STDERR,
  LM_SYSLOG,
};

enum log_level
{
  LL_CRITICAL,   /* application severed -> exit */
  LL_ERROR,      /* unwanted event but application can handle */
  LL_WARN,       /* unexpected event with potential to lead to errors */
  LL_INFO,       /* remarkable event */
  LL_EVENT,      /* standard event */
  LL_DEBUG,      /* additional output for error tracking */
  LL_DEBUG_MORE, /* even more additional output */
  LL_DEBUG_MAX,  /* all available additional output */

  LL_COUNT
};

enum log_facility
{
  LF_NONE = 0x00,
  LF_KERN       , /* kernel messages */
  LF_USER       , /* random user-level messages */
  LF_MAIL       , /* mail system */
  LF_DAEMON     , /* system daemons */
  LF_AUTH       , /* security/authorization messages */
  LF_SYSLOG     , /* messages generated internally by syslogd */
  LF_LPR        , /* line printer subsystem */
  LF_NEWS       , /* network news subsystem */
  LF_UUCP       , /* UUCP subsystem */
  LF_CRON       , /* clock daemon */
  LF_AUTHPRIV   , /* security/authorization messages (private) */
  LF_FTP        , /* ftp daemon */
  LF_LOCAL0     , /* reserved for local use */
  LF_LOCAL1     , /* reserved for local use */
  LF_LOCAL2     , /* reserved for local use */
  LF_LOCAL3     , /* reserved for local use */
  LF_LOCAL4     , /* reserved for local use */
  LF_LOCAL5     , /* reserved for local use */
  LF_LOCAL6     , /* reserved for local use */
  LF_LOCAL7     , /* reserved for local use */

  LF_COUNT
};

#ifndef MAX_LOG_LEN
#define MAX_LOG_LEN 256
#endif

#ifndef LOG_INFO /* syslog uses similar macro names for log levels */
#define LOG_EVENT(FORMAT, ...) log_push(LL_EVENT, FORMAT, ##__VA_ARGS__)
#define LOG_INFO(FORMAT, ... ) log_push(LL_INFO, FORMAT, ##__VA_ARGS__)
#define LOG_WARN(FORMAT, ...) log_push(LL_WARN, FORMAT, ##__VA_ARGS__)
#define LOG_ERROR(FORMAT, ...) log_push(LL_ERROR, FORMAT, ##__VA_ARGS__)
#define LOG_CRITICAL(FORMAT, ...) log_push(LL_CRITICAL, FORMAT, ##__VA_ARGS__)
#endif

#ifndef NULL
#  define NULL ((void*) 0)
#endif

#ifndef FALSE
#  define FALSE  0
#endif

#ifndef TRUE
#  define TRUE   (!0)
#endif


void log_init(enum log_mode mode, const char * ident, enum log_facility facility);
void log_set_level(enum log_level ll, size_t active);
int  log_get_level(enum log_level ll);
void log_push(const enum log_level ll, const char * format, ...) __attribute__((format(printf, 2, 3)));








#endif  /* _TOOL_LOGGER__H */
