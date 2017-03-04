#! /usr/bin/env python2
# -*- coding: utf-8 -*-

import paho.mqtt.client as mqtt
import datetime
import sys

from datetime import datetime

DAY_TARGET_TEMP_TOPIC = 'boiler/day_target_temp'
NIGHT_TARGET_TEMP_TOPIC = 'boiler/night_target_temp'
BOILER_ON_OFF_TOPIC = 'boiler/onoff'
DAY_SENSOR_TOPIC = 'temperature/living'
NIGHT_SENSOR_TOPIC = 'temperature/bedroom2'

SUBSCRIBE_MQTT_TOPICS = [DAY_SENSOR_TOPIC, NIGHT_SENSOR_TOPIC, DAY_TARGET_TEMP_TOPIC, NIGHT_TARGET_TEMP_TOPIC]

DAY_START_HOUR = 8
NIGHT_START_HOUR = 20

HYSTERESIS = 0.5

TURN_OFF = 0
TURN_ON = 1
NO_ACTION = -1

day_target_temp = None
night_target_temp = None
day_sensor_temp = None
night_sensor_temp = None

last_action = NO_ACTION


def boilerLogic(min, current, max):
    """
        Calculate and publish to BOILER_ON_OFF_TOPIC the required action based on min, max and current temperature.
    """
    if current <= min:
        print "The temperature is too low (current:%.1f <= min:%.1f)" % (current, min)
        current_action = TURN_ON
    elif current >= max:
        print "The temperature is too high (current:%.1f >= max:%.1f)" % (current, max)
        current_action = TURN_OFF
    else:
        print "The temperature is right (min:%.1f < current:%.1f < max:%.1f)" % (min, current, max)
        current_action = NO_ACTION

    # Avoid duplicated announces for the same action
    global last_action
    if last_action == current_action:
        return current_action

    if current_action == TURN_OFF or current_action == TURN_ON:
        print "Turning the boiler %d" % current_action
        client.publish(BOILER_ON_OFF_TOPIC, payload=current_action, qos=2, retain=True)
    else:
        print "No boiler action"

    last_action = current_action
    return current_action


def on_connect(client, userdata, flags, rc):
    """
        The callback for when the client receives a CONNACK response from the server.
    """
    print "Connected with result code %s" % str(rc)

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    for topic in SUBSCRIBE_MQTT_TOPICS:
        client.subscribe(topic)


def on_message(client, userdata, msg):
    """
        The callback for when a PUBLISH message is received from the server.
    """
    global day_target_temp
    global night_target_temp
    global day_sensor_temp
    global night_sensor_temp

    if msg.topic == DAY_TARGET_TEMP_TOPIC:
        day_target_temp = float(msg.payload)
        print "Target temperature set to: %.1f °C" % day_target_temp

    elif msg.topic == NIGHT_TARGET_TEMP_TOPIC:
        night_target_temp = float(msg.payload)
        print "Target temperature set to: %.1f °C" % night_target_temp

    elif msg.topic == DAY_SENSOR_TOPIC:
        day_sensor_temp = float(msg.payload)
        print "Day sensor temperature updated: %.1f °C" % day_sensor_temp

    elif msg.topic == NIGHT_SENSOR_TOPIC:
        night_sensor_temp = float(msg.payload)
        print "Night sensor temperature updated: %.1f °C" % night_sensor_temp

    now = datetime.now()
    todayDay = now.replace(hour=DAY_START_HOUR, minute=0, second=0, microsecond=0)
    todayNight = now.replace(hour=NIGHT_START_HOUR, minute=0, second=0, microsecond=0)

    if now > todayDay and now < todayNight:
        if not day_sensor_temp:
            print "No day temperature read"
            return
        if not day_target_temp:
            print "No day target temperature set"
            return
        min_temp = day_target_temp - HYSTERESIS
        max_temp = day_target_temp + HYSTERESIS
        sensor_temp = day_sensor_temp

    else:
        if not night_sensor_temp:
            print "No night temperature read"
            return
        if not night_target_temp:
            print "No night target temperature set"
            return
        min_temp = night_target_temp - HYSTERESIS
        max_temp = night_target_temp + HYSTERESIS
        sensor_temp = night_sensor_temp

    boilerLogic(min_temp, sensor_temp, max_temp)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
