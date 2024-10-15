# Files to deploy esp32 for beehive monitoring and disease control

- main.cpp file in src directory


# ESP32 Beehive Monitoring and Disease Control

This repository contains the files necessary to deploy an ESP32 microcontroller for beehive monitoring and disease control. 

## Requirements

* ESP32 development board
* Arduino IDE
* DHT22 temperature and humidity sensor and/or OneWire DS18B20 Temperature sensor
* 12V power supply
* Breadboard and jumper wires
* This code also supports a heater connected in the hive to maintain the temperatures. (Optional!)

## Setup

1. Install the Arduino IDE.
2. Connect the ESP32 development board to your computer via USB.
3. Select the appropriate board and port in the Arduino IDE.
4. Clone this repository into your Arduino IDE projects directory.
5. Open the `main.cpp` file in the Arduino IDE.
6. Enter your WiFi SSID and password in the `setup()` function.
7. Enter the IP address of your MQTT broker in the `setup()` function.
8. Enter the topic you want to publish data to in the `setup()` function.
9. Connect the  DHT22 sensor to the ESP32 board according to the following diagram:

```
DHT11 or DHT22  ESP32
VCC             3.3V
GND             GND
DATA            D4
```

10. Connect the DS18B20 temp. sensor to the ESP32 board according to the following diagram:

```
DS18B20       ESP32
VCC             3.3V
GND             GND
Data             5
```


12. Connect the 12V power supply to the ESP32 board.

## Usage

1. Open the Arduino IDE.
2. Click the "Upload" button to compile and upload the code to the ESP32 board.
3. Once the code has been uploaded, open the Serial Monitor.
4. You should see the ESP32 board print out its IP address and the MQTT broker IP address.
5. You should also see the ESP32 board print out the temperature, humidity, and air quality data.
