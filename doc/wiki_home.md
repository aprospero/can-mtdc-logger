# Abstract
This code project has the goal to expose the CAN-Bus protocol capabilities of Sorel M/LTDC controllers as throughoutly as possible.

# Short time goals
In a first step all available resource data about eg. sensors and relays is propagated to a MQTT broker.

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

# How it works
* The MTDC device is attached to the SoC board via CAN-bus. 
* This SW project provides a binary serving as translator between CAN-bus and MQTT, sending mosquitto anything about the state of the MTDC device.
* Telegraf in turn pipes data snippets of certain topics into influxdb, which is responsible for data persistence and answering queries.
* grafana queries influxdb for certain datasets, after configuring influxdb as data source and creating a matching dashboard.

# Result
Voila, one can use any browser to connect to the sabre lite SoC board and have a nice view over the state of the MTDC. 

![Grafana Dashboard](https://github.com/aprospero/can-mtdc-logger/blob/master/doc/grafana_dashboard.png)
