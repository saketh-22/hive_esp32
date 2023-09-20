
#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include "WiFi.h"
// #include "secrets.h"
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include <string>
#include <NTPClient.h>
#include <WiFiUdp.h>

// #define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
// #define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define DHTPIN 4      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

float h_dht, tc, tf;
float t_dht;

const char* WIFI_SSID = ""; //Enter SSID
const char* WIFI_PASSWORD = ""; //Enter Password
const char* websockets_server_host = ""; //Enter server adress
const uint16_t websockets_server_port = 8080; // Enter server port
const long utcOffsetInSeconds = 3600; // Central European Time (CET)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
// char path[] = "/";
// char host[] = "bee-service.onrender.com";

using namespace websockets;

WebsocketsClient client;
// WiFiClient client;

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

String server_data;

// Insert Firebase project API Key
#define API_KEY ""

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL ""
#define USER_PASSWORD ""

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL ""
// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempDsbPath = "/temperatureDsb";
String tempDhtPath = "/temperatureDht";
String humPath = "/humidity";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

String timestamp;
FirebaseJson json;


// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000;

// functions declaration
void heater();
float dsbtemp();
// void messageHandler(char *topic, byte *payload, unsigned int length);

// void messageHandler(char *topic, byte *payload, unsigned int length)
// {
//   Serial.print("incoming: ");
//   Serial.println(topic);
//   StaticJsonDocument<200> doc;
//   deserializeJson(doc, payload);
//   const char *message = doc["message"];
//   Serial.println(message);
// }

#define Time_To_Sleep 10   //Time ESP32 will go to sleep (in seconds)
#define S_To_uS_Factor 1000000ULL      //Conversion factor for micro seconds to seconds 

void firebaseConnect() {
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

}

String getTime() {
  // Initialize NTP client
  
  timeClient.begin();
  timeClient.update();

  // Get the current time
  String timeStamp = timeClient.getFormattedTime();
  return timeStamp;
}

void sendToFirebase(const std::string& tempDsb, const std::string& tempDht, const std::string& humDht) {
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set(tempDsbPath.c_str(), tempDsb);
    json.set(tempDhtPath.c_str(), tempDht);
    json.set(humPath.c_str(), humDht);
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
void setup()
{
  Serial.begin(115200);

  esp_sleep_enable_timer_wakeup(Time_To_Sleep * S_To_uS_Factor);
  Serial.println("Setup ESP32 to sleep for every " + String(Time_To_Sleep) + " Seconds");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.print("Wifi Connected !!!");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  //connectAWS();
  
  firebaseConnect();

  Serial.println("Connected to Wifi, Connecting to server.");
  // try to connect to Websockets server
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
  if(connected) {
      Serial.println("Connected!");
      client.send("Hello Server");
  } else {
      Serial.println("Not Connected!");
  }
  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message){
      Serial.print("Got Message: ");
      Serial.println(message.data());
      server_data = message.data();
      Serial.print("ServerData: ");
      Serial.print(server_data);

  });

  
  pinMode(ledPin, OUTPUT);

  // dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  dht.begin();
  sensor_t sensor;

  sensors.begin();
  delayMS = sensor.min_delay / 1000;
}

void loop()
{
  // get websocket msgs
  if(client.available()) {
        client.poll(); 
    }else {
      bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
      if(connected) {
          Serial.println("Connected!");
          client.send("Hello Server");
      } else {
          Serial.println("Not Connected!");
      }
    }
  
  
  // % fetch detection data
  // if mite-detection=true --> start heat treatment

  // DHT22 data
  // readDht();
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
    t_dht = event.temperature;
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
    h_dht = event.relative_humidity;
  }
  Serial.println(" ");
// End DHT22 reading ........................................................................

  float dsbT = dsbtemp();
  Serial.println("#################");
  Serial.println("Temp from OneWire sensor :");
  Serial.println(dsbT);
// End onew wire reading ..................................................................... 

sendToFirebase(std::to_string(dsbT),std::to_string(t_dht),std::to_string(h_dht));

// condition for heating process 
  if (dsbT >= 29) //Here 35 is adjusted as trigger to proceed to AI detection algorithm
  {
   // WebSocket setup
   if (client.available()) {
   client.send("Capture");
   Serial.print("Sent capture signal to Server");
   while(true){
    // webSocketClient.getData(server_data);
    if(client.available()) {
        client.poll(); 
    }
    Serial.print("received response:");
    Serial.print(server_data);
    if(server_data == "0" || server_data == "1")
    {break;}
    
    }
      
   }
      // Now that we have received the response, act on the treatment status
      
  String treatmentstatus = server_data;
  if (treatmentstatus == "0") {
    // Treatment status is false
     Serial.print("No disease found");
  } else {
    // Treatment status is true
    Serial.print("Disease present, Starting treatment");
    //heater();
    delay(50000);
    Serial.print("Treatment finished");
  }
    
  
}
// delay(5000); // Wait for a new reading from the sensor (DHT22 has ~0.5Hz sample rate)
esp_deep_sleep_start();
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