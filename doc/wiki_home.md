# Abstract

This project has the goal to expose the CAN-Bus protocol (further called SCBI Protocol) capabilities of Sorel M/LTDC controllers as throughoutly as possible.

Therefore it provides a low profile library [Sorella™](./lib_help.md) which is designed to be used on embedded systems to translate SCBI messages (probably received via CAN bus) in discrete device parameters, represented by type, name and value.

Furthermore it provides an example application for (Embedded) Linux based systems utilizing Sorella™ which makes use of Linux' device driver layer to access CAN bus, screen output and HDD/SDD, plus an instance of Mosquitto, a small and fast MQTT broker.

# Short time goals

Already accomplished:

* exhibit all parameters from datalogger-monitor messages namely sensors, relays and statistical data (overview) about yield over time. 

* bus management (control) messages ('I'm alive','who's there', etc.) are logged

* heating circuit control (hcc) messages are evaluated and logged

* standalone [Sorella™](./lib_help.md) library with a low footprint regarding prerequisites. If no linux environment is present a compatibility header provides necessary definitions.

* Linux compatible [application](./tool_help.md) to publish SCBI parameters to a mosquitto mqtt broker.

Yet to accomplish:

* SCBI library (see the API docs)
  
  * implement evaluation of still unsupported messages
  
  * write parameter support

* Linux application
  
  * parameter registration and application configuration via config file
  
  * make mqtt connection secure

# Setup

My personal setup is the following:

* HW: Freescale imx6 based SoC board 'sabre lite' - quad core ARM cortex a9 @ 800MHz

* SW:
  
  * OS:
    
    * yocto built freescale BSP (kirkstone) Image: core-image-base
  
  * SW: 
    
    * mosquitto (MQTT broker)
    * telegraf (server agent)
    * influxdb (time series database engine)
    * grafana (browser based dashboard creation)

This setup is one of many was to go ahead. It is eg. possible to fire up an Arduino or Raspberry PI with CAN support and just use the SCBI library or - in case of Linux use the Linux application in conjunction with an installed mosquitto instance.

# How my setup works

* The MTDC device is attached via CAN-bus to the SoC board. 
* The Linux [application](./tool_help.md) is serving as translator between SCBI protocol and MQTT, sending mosquitto every parameter change of the MTDC device.
* Telegraf in turn pipes data snippets of certain topics into influxdb, which is responsible for data persistence and answering queries.
* grafana queries influxdb for certain datasets, after configuring influxdb as data source and creating a matching dashboard.

# Result

Voila, one can use any browser to connect to the sabre lite SoC board and have a nice view over the state of the MTDC. 

![Grafana Dashboard](https://github.com/aprospero/can-mtdc-logger/blob/master/doc/grafana_dashboard.png)
