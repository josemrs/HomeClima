#! /usr/bin/env python2
# -*- coding: utf-8 -*-

import xively
import paho.mqtt.client as mqtt
import time
import sys

from datetime import datetime

MQTT_TOPICS = [
		'temperature/bedroom1',
		'temperature/bedroom2',
 		'temperature/living',
		'humidity/bedroom1',
		'pressure/bedroom1',
		'pressure/bedroom2',
		'pressure/living',
		'boiler/target_temp',
		'boiler/onoff'
	      ]
FEED_INTERVAL = 5*60

api = xively.XivelyAPIClient("xxxx")
feed = api.feeds.get(xxxx)

topicLastFeed = {}

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	global topicLastFeed
	print("Connected with result code "+str(rc))

	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	for topic in MQTT_TOPICS:
		client.subscribe(topic)
		topicLastFeed[topic] = 0

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	global topicLastFeed
	print(msg.topic+" "+str(msg.payload))

	if time.time() - topicLastFeed[msg.topic] > FEED_INTERVAL: 
		now = datetime.utcnow()
		payload = msg.payload
		id = msg.topic.split('/')[0] + '_' + msg.topic.split('/')[1]
		feed.datastreams = [
		    xively.Datastream(id, current_value=payload, at=now),
		]
		print('Feeding Xively now: ' + id + " " + str(payload))
		try:
			feed.update()
			topicLastFeed[msg.topic] = time.time()
		except:
			print "Error during feeding: ", sys.exc_info()[0]
			

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()

