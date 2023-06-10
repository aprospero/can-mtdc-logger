#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

char tmbuf[128];

char outbuf[128];


const char * getTimeValString(struct timeval tv, const char * format, char * buf, size_t buflen)
{
  time_t nowtime = tv.tv_sec;
  struct tm *   nowtm = localtime(&nowtime);

  if (format == NULL)
    format = "%Y-%m-%d %H:%M:%S";

  if (buf == NULL || buflen == 0)
  {
    buf = outbuf;
    buflen = sizeof(outbuf);
  }

  strftime(tmbuf, sizeof(tmbuf), format, nowtm);
  snprintf(buf, buflen, "%s.%06ld", tmbuf, tv.tv_usec);
  return buf;
}
