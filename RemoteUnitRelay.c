/* 
 * RemoteUnit for Multi Zone Wireless Thermostat
 * 
 * https://hackaday.io/project/19365-multi-zone-wireless-thermostat
 * 
*/

#include "LocalConfig.h"
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>  // Default: SDA pin: ESP8266 GPIO4 -> NodeMcu D2. SCL pin: ESP8266 GPIO5 -> NodeMcu D3.
                              // Wire.begin(12, 14); ESP8266 GPIO12 -> NodeMcu D6. SCL pin: ESP8266 GPIO14 -> NodeMcu D5.

static const uint16_t RESTART_DELAY        {5000};
static const uint16_t MAIN_LOOP_DELAY      {1000};
static const uint16_t MQTT_REPORT_INTERVAL {10}; // 10 * MAIN_LOOP_DELAY 

/* PIN definitions */
//const int8_t OUTPUT_PIN = 11;    // ESP8266 GPIO9 -> NodeMcu SD2
//const int8_t OUTPUT_PIN = 15;  // ESP8266 GPIO15 -> NodeMcu D8
const int8_t OUTPUT_PIN = 14;  // ESP8266 GPIO14 -> NodeMcu D5
//const int8_t OUTPUT_PIN = 10;  // ESP8266 GPIO10 -> NodeMcu SD3

/* Limits */
static const uint32_t MAX_TEMPERATURE {50};
static const uint32_t MIN_TEMPERATURE {10};
static const uint32_t MAX_HUMIDITY    {95};
static const uint32_t MIN_HUMIDITY    {5};

/* Strings */
static const String BEDROOM    {"Bedroom 2"};
static const String LIVING     {"Living room"};
static const char   OK_STR[]   {"Ok"};
static const char   FAIL_STR[] {"Failed"};

/* WiFi */
WiFiClient mqttWiFiClient;
WiFiClient webWiFiClient;
WiFiServer server(80);
IPAddress bedroom2Ip(192, 168, 1, 62);
IPAddress livingIp(192, 168, 1, 60);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(0,0,0,0);
IPAddress dns1(0,0,0,0);

/* MQTT */
static const char     MQTT_SERVER[]        {"192.168.1.15"};
static const uint32_t MQTT_PORT            {1883};
static const uint32_t MQTT_CONNECT_RETRIES {3};
Adafruit_MQTT_Client mqttClient(&mqttWiFiClient, MQTT_SERVER, MQTT_PORT);
uint32_t mqttCountdown {0};

/* MQTT Feeds */
Adafruit_MQTT_Publish temperatureTopic = Adafruit_MQTT_Publish(&mqttClient, "temperature/bedroom2");
Adafruit_MQTT_Publish pressureTopic = Adafruit_MQTT_Publish(&mqttClient, "pressure/bedroom2");
Adafruit_MQTT_Publish humidityTopic = Adafruit_MQTT_Publish(&mqttClient, "humidity/bedroom2");
Adafruit_MQTT_Subscribe outputTopic = Adafruit_MQTT_Subscribe(&mqttClient, "output/bedroom2");

/* BME sensor via I2C */
Adafruit_BME280 bme;
float temperature {-1};
float humidity {-1};
float pressure {-1};
static const int8_t MAX_ERROR_RETRIES {3};
int8_t errorRetries {0};

/* Error status */
static const uint8_t NO_ERROR        {0};
static const uint8_t MQTT_CONN_ERROR {1};
static const uint8_t MQTT_PUB_ERROR  {1 << 1};
uint8_t errorStatus {NO_ERROR};

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
bool MQTT_connect();

void restart()
{
  Serial.println("ESP restarting...");
  delay(RESTART_DELAY);
  ESP.restart();
}

void publishTopic(Adafruit_MQTT_Publish* topic, float value)
{
  Serial.print("Publish: ");
  if (!topic->publish(value))
  {
    Serial.println(FAIL_STR);
    errorStatus |= MQTT_PUB_ERROR;
  }
  else
  {
    Serial.println(OK_STR);
  }
}

void setup()
{
  Wire.begin(12, 14);
  //pinMode(OUTPUT_PIN, OUTPUT);
  //digitalWrite(OUTPUT_PIN, LOW);

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Sensor test... ");
  if (!bme.begin(0x76))
  {
    Serial.println("BME280@0x76 not detected");
    restart();
  }
  else
  {
    Serial.println("BME280@0x76 ready");
  }

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.print(WLAN_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  WiFi.config(bedroom2Ip, gateway, subnet);

  Serial.print(" connected. ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
  Serial.println("Web server started");

  mqttClient.subscribe(&outputTopic);

  Serial.println("Entering main loop");

#ifdef NO_SERIAL_OUTPUT
  Serial.println("Serial output disabled for main loop.");
  delay(5000);
  yield();
  Serial.end();
#endif
}

String buildHttpResponse()
{
    // Prepare the response. Start with the common header:
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n\r\n";
    response += "<!DOCTYPE HTML>\r\n<html><body>";

    response += "<h1>" + BEDROOM + "</h1>";
    response += "<br>Temperature: " + String(temperature);
    response += "<br>Humidity: " + String(humidity);
    response += "<br>Pressure: " + String(pressure);
    response += "<br><br>Next report in: " + String((mqttCountdown*MAIN_LOOP_DELAY)/1000) + "s";
    response += "<br>Error status: " + String(errorStatus);

    if (errorStatus != NO_ERROR)
    {
      response += "&emsp;<input type=\"button\" onclick=\"location.href='http://" + WiFi.localIP().toString() + "/clearErrorStatus';\" value=\"Clear error status\" />";
    }

    response += "<br><br><input type=\"button\" onclick=\"location.href='http://" + WiFi.localIP().toString() + "/restart';\" value=\"Restart ESP\" />";
    response += "\r\n</body></html>\n";

    return response;
}

void loop()
{ 
  if (mqttCountdown <= 0)
  {
    mqttCountdown = MQTT_REPORT_INTERVAL;
    Serial.println();

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected)
    bool mqtt_connected = MQTT_connect();

    if (mqtt_connected)
    {
      Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqttClient.readSubscription(1000)))
      {
        if (subscription == &outputTopic)
        {
          if (strcmp((char *)outputTopic.lastread, "1") == 0)
          {
            Serial.println("Turning output ON");
            digitalWrite(OUTPUT_PIN, HIGH);
          }
          else
          {
            Serial.println("Turning output OFF");
            digitalWrite(OUTPUT_PIN, LOW);
          }
        }
      }
    }
    else
    {
      errorStatus |= MQTT_CONN_ERROR;
    }
    
    temperature = bme.readTemperature();
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C    ");

    bool outOfLimits = false;
    if (temperature < MIN_TEMPERATURE || temperature > MAX_TEMPERATURE || isnan(temperature))
    {
        Serial.println("Out of limits.");
        outOfLimits = true;
    }
    else if (mqtt_connected)
    {
        publishTopic(&temperatureTopic, temperature);
    }

    humidity = bme.readHumidity();
    Serial.print("Humidity:    ");
    Serial.print(humidity);
    Serial.print(" %     ");

    if (humidity < MIN_HUMIDITY || humidity > MAX_HUMIDITY || isnan(humidity))
    {      
        Serial.println("Out of limits.");
        outOfLimits = true;
    }
    else if (mqtt_connected)
    {
        errorRetries = 0;
        publishTopic(&humidityTopic, humidity);
    }

    pressure = bme.readPressure() / 100.0F;
    Serial.print("Pressure:    ");
    Serial.print(pressure);
    Serial.print(" hPa ");

    if (isnan(pressure))
    {      
        Serial.println("Out of limits.");
        outOfLimits = true;
    }
    else if (mqtt_connected)
    {
        publishTopic(&pressureTopic, pressure);
    }
       
    if (outOfLimits)
    { 
        errorRetries++;
        Serial.print("ERROR: Out of limits detected, ");
        Serial.print(MAX_ERROR_RETRIES - errorRetries);
        Serial.println(" retries to reboot.");
        if (errorRetries >= MAX_ERROR_RETRIES)
        {
           restart();
        }
    }
    else
    {
        errorRetries = 0;
    }    
  }
  --mqttCountdown;
  delay(MAIN_LOOP_DELAY);

  webWiFiClient = server.available();
  if (webWiFiClient)
  {
    // Read the first line of the request
    String req = webWiFiClient.readStringUntil('\r');
    webWiFiClient.flush();
    if (req.indexOf("/clearErrorStatus") != -1)
    {
        Serial.println("Error status clear requested from web interface");
        errorStatus = NO_ERROR;
    }
    if (req.indexOf("/restart") != -1)
    {
        Serial.println("Reset requested from web interface");
        ESP.restart();
    }

    // Prepare the response. Start with the common header:
    String response = buildHttpResponse();

    webWiFiClient.flush();

    // Send the response to the client
    webWiFiClient.print(response);
  }  
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqttClient.connected())
  {
    return true;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = MQTT_CONNECT_RETRIES;
  while ((ret = mqttClient.connect()) != 0)  // connect will return 0 for connected
  {
    Serial.println(mqttClient.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqttClient.disconnect();
    delay(5000);
    retries--;
    if (retries == 0)
    {
      Serial.println(FAIL_STR);
      return false;
    }
  }

  Serial.println(OK_STR);
  return true;
}
