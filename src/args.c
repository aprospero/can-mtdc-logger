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

  config->prg_name = strrchr(argv[0], '/');
  if (config->prg_name == NULL)
    config->prg_name = argv[0];
  else
    config->prg_name++;

  config->log_facility = DEFAULT_LOG_FACILITY;
  config->log_level    = DEFAULT_LOG_LEVEL;
  config->can_device   = DEFAULT_CAN_DEVICE;

  config->mqtt.remote_address = DEFAULT_MQTT_REMOTE;
  config->mqtt.remote_port    = DEFAULT_MQTT_PORT;
  config->mqtt.client_id      = DEFAULT_MQTT_CLIENT_ID;
  config->mqtt.topic          = DEFAULT_MQTT_TOPIC;
  config->mqtt.qos            = DEFAULT_MQTT_QOS;

  while ((opt = getopt(argc, argv, "hf:Vv:d:m:r:p:i:t:q:")) != -1)
  {
    switch (opt)
    {
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
      case 'r':
      {
        config->mqtt.remote_address = optarg;
        if (*optarg == '\0') {
          fprintf(stderr, "Error: empty MQTTY remote address.\n");
          goto ON_ERROR;
        }
        break;
      }
      case 'p':
      {
        config->mqtt.remote_port = strtol(optarg, NULL, 0);
        if (config->mqtt.remote_port < 1 || config->mqtt.remote_port > 65535) {
          fprintf(stderr, "Error: invalid MQTT remote port.\n");
          goto ON_ERROR;
        }
        break;
      }
      case 'i':
      {
        config->mqtt.client_id = optarg;
        if (*optarg == '\0') {
          fprintf(stderr, "Error: empty MQTT client_id.\n");
          goto ON_ERROR;
        }
        break;
      }
      case 't':
      {
        config->mqtt.topic = optarg;
        if (*optarg == '\0') {
          fprintf(stderr, "Error: empty MQTTY topic.\n");
          goto ON_ERROR;
        }
        break;
      }
      case 'q':
      {
        config->mqtt.qos = strtol(optarg, &end, 0);
        if (config->mqtt.qos < 0 || config->mqtt.remote_port > 2 || end == optarg) {
          fprintf(stderr, "Error: invalid QoS.\n");
          goto ON_ERROR;
        }
        break;
      }

      case 'h':
      {
        goto ON_HELP;
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
  fprintf(err ? stderr : stdout, "usage: %s [-hV] [-d <can-device>] [-r <mqtt remote address>] [-p <mqtt remote port>] [-i <mqtt client-id>] [-t <mqtt topic>] [-q <mqtt QoS>] [-v <log level>] [-f <log facility>]\n", config->prg_name);
  if (err)
    exit(1);
  fprintf(stdout, "\nOptions:\n");
  fprintf(stdout, "  -d: CAN bus device. Default is: " DEFAULT_CAN_DEVICE "\n");
  fprintf(stdout, "  -v: verbosity information. Available log levels:\n");
  for (idx = 1; idx < LL_COUNT; idx++)
    fprintf(stdout, "%s%s%s", log_get_level_name((enum log_level) idx, TRUE), idx == DEFAULT_LOG_LEVEL ? " (default)" :  "",  idx < LL_COUNT - 1 ? (idx - 1) % 8 == 7 ? ",\n" : ", " : ".\n");
  fprintf(stdout, "  -f: Log Facility. Available log facilities:\n");
  for (idx = 0; idx < LF_COUNT; idx++)
    fprintf(stdout, "%s%s%s", log_get_facility_name((enum log_facility) idx), idx == DEFAULT_LOG_FACILITY ? " (default)" :  "", idx < LF_COUNT - 1 ? idx % 8 == 7 ? ",\n" : ", " : ".\n");

  fprintf(stdout, "  -r: MQTT brokers remote IP address or server name. Default is: " DEFAULT_MQTT_REMOTE "\n");
  fprintf(stdout, "  -p: MQTT brokers remote port. Default is: %d\n", DEFAULT_MQTT_PORT);
  fprintf(stdout, "  -i: MQTT client id (also used as user name). Default is: " DEFAULT_MQTT_CLIENT_ID "\n");
  fprintf(stdout, "  -t: MQTT topic. Default is: " DEFAULT_MQTT_TOPIC "\n");
  fprintf(stdout, "  -q: MQTT quality of service. Default is: %d\n", DEFAULT_MQTT_QOS);

  fprintf(stdout, "  -h: Print usage information and exit\n");
  fprintf(stdout, "  -V: Print version information and exit\n");
  fflush(stdout);
  exit(0);

ON_VERSION:
  fprintf(stdout, "%s V" APP_VERSION "\n", config->prg_name);
  fflush(stdout);
  exit(0);
}
