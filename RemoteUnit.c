/* 
 * RemoteUnit for Multi Zone Wireless Thermostat
 * 
 * https://hackaday.io/project/19365-multi-zone-wireless-thermostat
 * 
*/

#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>  // SDA pin: ESP8266 GPIO4 -> NodeMcu D2.
                              // SCL pin: ESP8266 GPIO5 -> NodeMcu D3. 

const uint16_t MAIN_LOOP_DELAY = 10000;

/* PIN definitions */
const int8_t OUTPUT_PIN = 10;   // ESP8266 GPIO10 -> NodeMcu SD3

/* WiFi */
#define WLAN_SSID "****"
#define WLAN_PASS "****"
WiFiClient client;

/* MQTT */
#define MQTT_SERVER  "192.168.1.15"
#define MQTT_PORT    1883
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT);

/* MQTT Feeds */
Adafruit_MQTT_Publish temperature_topic = Adafruit_MQTT_Publish(&mqtt, "temperature/living");
Adafruit_MQTT_Publish pressure_topic = Adafruit_MQTT_Publish(&mqtt, "pressure/living");
Adafruit_MQTT_Publish humidity_topic = Adafruit_MQTT_Publish(&mqtt, "humidity/living");
Adafruit_MQTT_Subscribe boiler_onoff = Adafruit_MQTT_Subscribe(&mqtt, "boiler/onoff");

/* BME sensor via I2C */
Adafruit_BME280 bme;

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup()
{
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);

  Serial.begin(115200);
  delay(10);

  Serial.println("Sensor Test");
  if (!bme.begin(0x76))
  {
    Serial.println("BME280@0x76 not detected");
    if (!bme.begin(0x77))
    {
      Serial.println("BME280@0x77 not detected");
      delay(1);
      while (1);
    }
  }
  else
  {
    Serial.println("BME280 ready.");
  }

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt.subscribe(&boiler_onoff);
}

void loop()
{

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected)
  MQTT_connect();
  
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000)))
  {
    if (subscription == &boiler_onoff)
    {
      if (strcmp((char *)boiler_onoff.lastread, "ON") == 0)
      {
        Serial.println("Turning boiler ON");
        digitalWrite(OUTPUT_PIN, HIGH);
      }
      else
      {
        Serial.println("Turning boiler OFF");
        digitalWrite(OUTPUT_PIN, LOW);
      }
    }
  }

  float temperature = bme.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  float pressure = bme.readPressure() / 100.0F;
  Serial.print("Pressure:    ");
  Serial.print(pressure);
  Serial.println(" hPa");

  float humidity = bme.readHumidity();
  Serial.print("Humidity:    ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Publish Temperature: ");
  if (!temperature_topic.publish(temperature))
  {
    Serial.println("Failed");
  }
  else
  {
    Serial.println("OK");
  }

  Serial.print("Publish Pressure: ");
  if (!pressure_topic.publish(pressure))
  {
    Serial.println("Failed");
  }
  else
  {
    Serial.println("OK");
  }

  Serial.print("Publish Humidity: ");
  if (! humidity_topic.publish(humidity))
  {
    Serial.println("Failed");
  }
  else
  {
    Serial.println("OK!");
  }

  delay(MAIN_LOOP_DELAY);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}
