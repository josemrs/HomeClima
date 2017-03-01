#! /usr/bin/env python2
# -*- coding: utf-8 -*-

# Updating a feed
import paho.mqtt.client as mqtt
import datetime
import sys

from datetime import datetime

TARGET_TEMP_TOPIC='boiler/target_temp'
DAY_SENSOR='temperature/living'
NIGHT_SENSOR='temperature/bedroom2'
BOILER_ONOFF='boiler/onoff'

MQTT_TOPICS = [DAY_SENSOR, NIGHT_SENSOR, TARGET_TEMP_TOPIC]
DAY_START_HOUR = 8
NIGHT_START_HOUR = 20

HYSTERESIS = 0.5

target_temp = None
day_sensor_temp = None
night_sensor_temp = None

TURN_OFF = 1
TURN_ON = -1
NO_ACTION = 0

last_action = NO_ACTION

def boilerLogic(min, current, max):

	if current <= min:
		print "The temperature is too low (current:%.1f <= min:%.1f)" % (current, min)
		current_action = TURN_ON
	elif current >= max:
		print "The temperature is too high (current:%.1f => max:%.1f)" % (current, max)
		current_action = TURN_OFF
	else:
		print "The temperature is right (min:%.1f < current:%.1f < max:%.1f)" % (min, current, max)
		current_action = NO_ACTION

	global last_action
	if last_action == current_action:
		return current_action

	if current_action == TURN_OFF:
		print "Turning the boiler OFF"
		client.publish(BOILER_ONOFF, payload='OFF', qos=2, retain=True)
	elif current_action == TURN_ON:
		print "Turning the boiler ON"
		client.publish(BOILER_ONOFF, payload='ON', qos=2, retain=True)

	last_action = current_action
	return current_action


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))

	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	for topic in MQTT_TOPICS:
		client.subscribe(topic)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	global target_temp
	global day_sensor_temp
	global night_sensor_temp

	if msg.topic == TARGET_TEMP_TOPIC:
		target_temp = float(msg.payload)
#		print "Target temperature set to: %.1f °C" % target_temp

	elif msg.topic == DAY_SENSOR:
		day_sensor_temp = float(msg.payload)
#		print "Day sensor temperature updated: %.1f °C" % day_sensor_temp

	elif msg.topic == NIGHT_SENSOR:
		night_sensor_temp = float(msg.payload)
#		print "Night sensor temperature updated: %.1f °C" % night_sensor_temp

	if not target_temp:
		print "No target temperature set"
		return

	min_temp = target_temp - HYSTERESIS
	max_temp = target_temp + HYSTERESIS

	now = datetime.now()
	todayDay = now.replace(hour=DAY_START_HOUR, minute=0, second=0, microsecond=0)
	todayNight = now.replace(hour=NIGHT_START_HOUR, minute=0, second=0, microsecond=0)

	if now > todayDay and now < todayNight:
		if not day_sensor_temp:
			print "No day temperature read"
			return

		boilerLogic(min_temp, day_sensor_temp, max_temp)
	else:
		if not night_sensor_temp:
			print "No night temperature read"
			return

		boilerLogic(min_temp, night_sensor_temp, max_temp)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()

