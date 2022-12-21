#ifndef _H_ARGS
#define _H_ARGS

#include "tool/logger.h"


#define DEFAULT_LOG_FACILITY LF_LOCAL0
#define DEFAULT_LOG_LEVEL    LL_ERROR

#define DEFAULT_CAN_DEVICE "/dev/can0"


struct cansorella_config
{
    const char *      prg_name;
    enum log_facility log_facility;
    enum log_level    log_level;
    char *            can_device;
};

int parseArgs(int argc, char * argv[], struct cansorella_config * config);


#endif
