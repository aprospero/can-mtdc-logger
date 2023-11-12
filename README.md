# Quick Introduction

This project aims to fully support native communication with MTDC/LTDC devices via the SCBI (Sorel CAN bus Interface) protocol by parsing and translating MTDC/LTDC CAN bus frames into distinct parameters and (hopefully one day) generating messages to control the state of the device.



The Project consists of **Sorella™**, a low level C module/library that provides the ability to parse MTDC/LTDC CAN bus messages, and **CanSorella™** that combines (embedded) Linux and Sorella to translate incoming MTDC/LTDC CAN bus frames in MQTT parameter messages.



CAN bus frame interpretation is guided by SOREL CAN BUS Interface (SCBI) documentation. Many Thanks to the superb suppport at sorel.de, namely international technical supporter Rhona Mackay who always patiently answered any question/request!

For further info about this project have a look into the [docs](doc/wiki_home.md)
