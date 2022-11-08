#!/bin/bash

MQTT_SERVER=<PUT SERVERNAME HERE>
MQTT_USER=<PUT USERNAME HERE>
MQTT_PASSWORD=<PUT PASSWORD HERE>

IFS=";"
can-ltdc-logger | while read  timestamp program name value extra; do
	echo Read: $timestamp $program $name $value $extra
	if [ "$program" == "DLG_RELAY" ]; then
		mosquitto_pub -u $MQTT_USER -P $MQTT_PASSWORD -h $MQTT_SERVER -m "$extra"  -t "ltdc/$program/$name/$value" -q 1 -r
	else
		mosquitto_pub -u $MQTT_USER -P $MQTT_PASSWORD -h $MQTT_SERVER -m "$value"  -t "ltdc/$program/$name" -q 1 -r
	fi
done
