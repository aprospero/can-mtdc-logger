#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl/scbi_api.h"
#include "version.h"

void log_fn(enum scbi_log_level ll, const char * format, ...)
{
  if (ll < SCBI_LL_CNT)
  {
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\n");
  }
}

static const char * param_type_translate[] = {
    "sensor",    /* SCBI_PARAM_TYPE_SENSOR     */
    "relay",     /* SCBI_PARAM_TYPE_RELAY      */
    "overview"   /* SCBI_PARAM_TYPE_OVERVIEW   */
};

/* line examples:
 *   2   6              20          33  36
 *  (000.099732)  can0  10019F85   [1]  01
 *  (000.100005)  can0  00019F85   [0]
 *  (000.100254)  can0  10079F80   [8]  80 00 4D 04 00 00 00 00
 *  (000.099904)  can0  10029F80   [5]  00 00 00 FF FF
 *
 *  command line: candump -td -d can0 > file.log
 */

#define STR2UINT32(START,END,RADIX) (end = &line[END] , strtoul(&line[START], &end, RADIX))

static void parse_line(struct scbi_frame * frame, char * line, scbi_time * cum)
{
  char * end;
  uint32_t value;
  printf("%s", line);
  value = STR2UINT32(2,5,10);
  *cum += value * 1000;
  value = STR2UINT32(6,12,10);
  *cum += value / 1000;

  frame->recvd = *cum;
  frame->msg.can_id = STR2UINT32(20,28,16);
  frame->msg.len = STR2UINT32(33,34,16);

  for (int i = 0; i < frame->msg.len; i++)
    frame->msg.data[i] = STR2UINT32(36 + i * 3, 37 + i * 3, 16);
}

static void parse_file(struct scbi_handle * hnd, const char * fname)
{
  struct scbi_frame frame;
  struct scbi_param * param;
  scbi_time cum = 0;
  static FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(fname, "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);

  while ((read = getline(&line, &len, fp)) != -1)
  {
    memset(&frame, 0, sizeof frame);
    parse_line(&frame, line, &cum);
//    scbi_print_frame(hnd, SCBI_LL_INFO, "TEST", "TOAST", &frame);
    if (scbi_parse(hnd,  &frame) == 0)
    {
      while ((param = scbi_pop_param(hnd)) != NULL)
        printf("Name: %s,   type: %s,  value: %u.\n", param->name, param_type_translate[param->type], param->value);
    }
  }
  fclose(fp);
}


int main(int argc, char * argv[])
{
  struct scbi_handle * scbi;

  const char * fname = argc > 1 ? argv[1] : "../dumps/can0.log";

  fprintf(stdout, "##########################################################################\n");
  fprintf(stdout, "Starting %s "APP_VERSION" - Input file:%s.\n", argv[0], fname);
  fprintf(stdout, "##########################################################################\n");

  scbi = scbi_init(malloc, log_fn, 300);
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

    parse_file(scbi, fname);

  }
	return 0;
}
