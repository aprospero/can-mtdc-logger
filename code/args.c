#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "version.h"


int parseArgs(int argc, char * argv[], struct cansorella_config * config)
{
  char * end;
  int logset = FALSE;
  int err = 0, idx, opt;
  optind = 1;

  config->prg_name = argv[0];

  config->prg_name = strrchr(config->prg_name, '/');

  if (config->prg_name == NULL)
    config->prg_name = argv[0];
  else
    config->prg_name++;

  config->log_facility = DEFAULT_LOG_FACILITY;
  config->log_level    = DEFAULT_LOG_LEVEL;
  config->can_device   = DEFAULT_CAN_DEVICE;

  while ((opt = getopt(argc, argv, "hb:f:l:Vv:w:s:g:c:a:d:")) != -1)
  {
    switch (opt)
    {
      case 'h':
      {
        goto ON_HELP;
      }
      case 'd':
      {
        if (*optarg == '\0')
        {
          fprintf(stderr, "Error: empty device name.\n");
          goto ON_ERROR;
        }
        config->can_device = optarg;
        break;
      }
      case 'v':
      {
        enum log_level ll = log_get_level_no(optarg);

        if (ll == LL_NONE)
        {
          fprintf(stderr, "Error: invalid log level stated (%s)\n", optarg);
          goto ON_ERROR;
        }
        config->log_level = ll;
        break;
      }
      case 'f':
      {
        enum log_facility lf = log_get_facility(optarg);

        if (lf == LF_COUNT)
        {
          fprintf(stderr, "Error: invalid log faciltiy stated (%s).\n", optarg);
          goto ON_ERROR;
        }
        config->log_facility = lf;
        break;
      }
      case 'V':
      {
        goto ON_VERSION;
      }
      default:
      {
        goto ON_ERROR;
      }
    }
  }
  return err;

ON_ERROR:
  err = 1;
ON_HELP:
  fprintf(err ? stderr : stdout, "usage: %s [-hV] [-d <can-device>] [-v <log level>] [-f log facility]\n", config->prg_name);
  if (err)
    exit(1);
  fprintf(stdout, "\nOptions:\n");
  fprintf(stdout, "  -d: CAN bus device. Default is: " DEFAULT_CAN_DEVICE "\n");
  fprintf(stdout, "  -v: verbosity information. Available log levels:\n");
  for (idx = 1; idx < LL_COUNT; idx++)
    fprintf(stdout, "%s%s%s", log_get_level_name((enum log_level) idx), idx == DEFAULT_LOG_LEVEL ? " (default)" :  "",  idx < LL_COUNT - 1 ? (idx - 1) % 8 == 7 ? ",\n" : ", " : ".\n");
  fprintf(stdout, "  -f: Log Facility. Available log facilities:\n");
  for (idx = 1; idx < LF_COUNT; idx++)
    fprintf(stdout, "%s%s%s", log_get_facility_name((enum log_facility) idx), idx == DEFAULT_LOG_FACILITY ? " (default)" :  "", idx < LF_COUNT - 1 ? idx % 8 == 7 ? ",\n" : ", " : ".\n");
  fprintf(stdout, "  -h: Print usage information and exit\n");
  fprintf(stdout, "  -V: Print version information and exit\n");
  fflush(stdout);
  exit(0);

ON_VERSION:
  fprintf(stdout, "%s V" APP_VERSION "\n", config->prg_name);
  fflush(stdout);
  exit(0);
}
