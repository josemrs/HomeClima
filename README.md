# Multi Zone Wireless Thermostat

Main web: https://hackaday.io/project/19365-multi-zone-wireless-thermostat

Replace the bimetallic thermostat with a few microcontroller based thermostats to control the combi boiler.

## Software requirements

* Arduino IDE
* Python

## Required Libraries

* ESP8266WiFi https://github.com/ekstrand/ESP8266wifi
* Adafruit_MQTT https://github.com/adafruit/Adafruit_MQTT_Library
* Adafruit_Sensor https://github.com/adafruit/Adafruit_Sensor
* Adafruit_BMP280 https://github.com/adafruit/Adafruit_BMP280_Library

## Folders and Files

* RemoteUnit.c C code ready to use in the Arduino IDE to program the ESP8266
* etc/openhab2 Contains my personal config (sitemap and items) for OpenHab2 (http://www.openhab.org/)
* controller Contains the scripts for the controller unit (RPi in my case)
  * boilerLogic.py Subscribe to temperatures and publishes to boiler topic. Implements the logic to turn on/off the boiler depending on temperatures, time and target.
  * xivelyFeeder.py Handy script to post MQTT topics to Xively (http://personal.xively.com) **OpenHab can do this instead**
