# CanSorella™

###### Usage:

```
cansorella [-hV] [-d <can-device>] 
           [-r <mqtt remote address>] [-p <mqtt remote port>] 
           [-i <mqtt client-id>] [-t <mqtt topic>] [-q <mqtt QoS>] 
           [-v <log level>] [-f <log facility>]
```

###### Options:

- **-d**  CAN bus device. Default is: /dev/can0

- **-r**  MQTT broker remote IP address or server name. Default: **localhost**
  
- **-p**  MQTT broker remote port. Default: **1183**
  
- **-i**  MQTT client id (also used as user name). Default: **cansorella**
  
- **-t**  MQTT topic. Default: **MTDC**
  
- **-q**  MQTT quality of service. Default: **2**

- **-v**  verbosity information. Available log levels: 
     CRITICAL, **ERROR** (default), WARNING, INFO, 
     EVENT, DEBUG, DEBUG_MORE, DEBUG_MAX.

- **-f**  Log Facility. Available log facilities:
     stdout, user, **local0** (default), 
     local1, local2, local3, local4, local5, local6, local7. 
     Facilities other than stdout make use of the linux syslog mechanism.

- **-h**  Print usage information and exit

- **-V**  Print version information and exit

###### Build environment

CanSorella™ depends on the [Sorella™ shared library](./lib_help.md)  and mosquitto, a tiny MQTT broker for Linux.

to date the git repo contains several eclipse cdt projects. They include source code resources via virtual folders and files which makes them independent from any other make/cmake/whatever buildsystem support possibly added in the future.

Nevertheless most likely other build systems will lack of the same cross-compile toolchain referenced in the provided eclipse projects, so it would need a complete reconfiguration of the build settings to be of use........ 
