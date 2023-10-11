# Cansorella

###### Usage:

    cansorella [-hV] [-d \<can-device\>] [-v \<log level\>] [-f log facility]

###### Options:

- -d: CAN bus device. Default is: /dev/can0

- -v: verbosity information. Available log levels:
  CRITICAL, ERROR (default), WARNING, INFO, EVENT, DEBUG, DEBUG_MORE, DEBUG_MAX.

- -f: Log Facility. Available log facilities:
  stdout, user, local0 (default), local1, local2, local3, local4, local5,
  local6, local7. 
  Facilities other than stdout make use of the syslog mechanism

- -h: Print usage information and exit

- -V: Print version information and exit

###### Hard coded options:

- MQTT Broker is expected at localhost:1883

- MQTT client id: 'cansorella'

- MQTT topic: 'MTDC'

- MQTT QoS: 2 
  
  TODO: make MQTT broker parameter available via cli option. In the meantime: have a look at [src/linuxtools/ctrl/com/mqtt.c](../../src/linuxtools/ctrl/com/mqtt.c) function name: mqtt_init

###### Build environment

to date an eclipse cdt project is part of the repo. It includes source code resources via virtual folders and files which makes it independent from any other make/cmake/whatever buildsystem support possibly added in the future.

Nevertheless most likely other build systems will lack of the same cross-compile toolchain referenced in the provided eclipse project, so it would need a complete reconfiguration of the build settings to be of use........ 
