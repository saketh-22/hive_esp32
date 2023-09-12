#include <Arduino.h>

// #include "DHTesp.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "PubSubClient.h"
#include "secrets.h"
#include <ArduinoJson.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define DHTPIN 4      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

float h, tc, tf;
float t;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

int hi = 41.5;
int lo = 40.5;
// DHTesp dhtSensor;
const int ledPin = 14;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 5;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// functions declaration
void heater();
float dsbtemp();
void messageHandler(char *topic, byte *payload, unsigned int length);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler

  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["Sensor"] = "DHT22";
  doc["humidity"] = h;
  doc["temperature"] = t;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(char *topic, byte *payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *message = doc["message"];
  Serial.println(message);
}

void readDht()
{
  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  Serial.println("#####DHT22 READINGS########");
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    t = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println(F("Error reading humidity!"));
  }
  else
  {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    h = event.relative_humidity;
  }
  Serial.println(" ");
}

void setup()
{
  Serial.begin(115200);
  connectAWS();
  pinMode(ledPin, OUTPUT);

  // dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  dht.begin();
  sensor_t sensor;

  sensors.begin();
  delayMS = sensor.min_delay / 1000;
}

void loop()
{
  // % fetch detection data
  // if mite-detection=true --> start heat treatment
  //
  bool treatmentstatus = false; // fetch from cloud/user

  if (treatmentstatus == true)
  {
    heater();
  }
  else
  {
    // heater off
    digitalWrite(ledPin, LOW);
  }

  // DHT22 data
  readDht();

  float dsbT = dsbtemp();

  Serial.println("#################");
  publishMessage();
  client.loop();
  delay(5000); // Wait for a new reading from the sensor (DHT22 has ~0.5Hz sample rate)
}

void heater()
{
  unsigned long startTime = millis(); // Record the start time
  unsigned long loopDuration = 50000; // Loop duration in milliseconds (50 seconds)
  Serial.println("Started Heating Treatment!!!");
  // loop to maintain temperature
  while (millis() - startTime < loopDuration)
  {
    float currtemp = dsbtemp();
    if (currtemp >= hi)
    {
      Serial.println("heater OFF");
      digitalWrite(ledPin, LOW);
    }
    else if (currtemp <= lo)
    {
      // Serial.print(temperatureC);
      // Serial.println("ºC");
      Serial.println("heater ON");
      digitalWrite(ledPin, HIGH);
    }
    delay(5000);
  }
  Serial.println("End heat Treatment!!!");
}

float dsbtemp()
{
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.println("DSB values");
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  return temperatureC;
}