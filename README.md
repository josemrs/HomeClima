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
* Adafruit_BME280 https://github.com/adafruit/Adafruit_BME280_Library

## Folders and Files

* RemoteUnit.c C code ready to use in the Arduino IDE to program the ESP8266
  * Reset to tag last_bmp280 for code with BMP280 support
* etc/openhab2 Contains my personal config (sitemap and items) for OpenHab2 (http://www.openhab.org/)
* controller Contains the scripts for the controller unit (RPi in my case)
  * boilerLogic.py Subscribe to temperatures and publishes to boiler topic. Implements the logic to turn on/off the boiler depending on temperatures, time and target.
  * xivelyFeeder.py (I'll not do any more work on this, I am not using it any more) Handy script to post MQTT topics to Xively (http://personal.xively.com)
  * carbonFeeder.py Handy script to post MQTT topics to Carbon for Grafana
