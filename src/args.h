#ifndef _H_ARGS
#define _H_ARGS

#include "ctrl/com/mqtt.h"
#include "ctrl/logger.h"

#define DEFAULT_LOG_FACILITY LF_LOCAL0
#define DEFAULT_LOG_LEVEL    LL_ERROR

#define DEFAULT_CAN_DEVICE "can0"

#define DEFAULT_MQTT_REMOTE "localhost"
#define DEFAULT_MQTT_PORT   1883

#define DEFAULT_MQTT_CLIENT_ID "cansorella"
#define DEFAULT_MQTT_TOPIC     "MTDC"
#define DEFAULT_MQTT_QOS       2


struct cansorella_config
{
    const char *       prg_name;
    enum log_facility  log_facility;
    enum log_level     log_level;
    char *             can_device;
    struct mqtt_config mqtt;
};

int parseArgs(int argc, char * argv[], struct cansorella_config * config);


#endif
