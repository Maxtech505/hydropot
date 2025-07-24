/* 
 * Project My hydropot
 * Author: Maximo Regalado
 * Date: 2025-07-17
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Run system automatically without cloud connection
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#include "Adafruit_BME280.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "neopixel.h"
#include "Colors.h"
#include "Air_Quality_Sensor.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"

TCPClient TheClient; 

Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY);
 
Adafruit_MQTT_Subscribe WaterButton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/waterbutton"); 
Adafruit_MQTT_Publish TEMP = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish HUMIDITY = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish MOISTURE = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/moisture");
Adafruit_MQTT_Publish AIRQUALITY = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/airquality");
Adafruit_MQTT_Publish WATERLEVEL = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/waterlevel");

int buttonState;
unsigned long publishTime;
void MQTT_connect();
bool MQTT_ping();
int readWaterLevelSensor();
void setupWiFi();

// Function to scan for I2C devices
void scanI2C() {
  Serial.println("Scanning for I2C devices...");
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("I2C device found at address 0x%02X\n", address);
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.printf("Found %d I2C device(s)\n", nDevices);
  }
  Serial.println();
}

int soilMoist= A1;  
int moistureReads;

const int sensorPin = A3;  
const int sensorPower = D3;  
int sensorValue = 0;
float waterLevelPercentage;

String dateTime, timeOnly;
unsigned int lastTime;
unsigned int lastPublish;

Adafruit_BME280 bme;
bool status;
const int hexAddress = 0x77; // Try 0x77 if 0x76 doesn't work
unsigned int currentTime;
unsigned int lastSecond;
float tempC;
float tempF;
float humidRH;
const byte PERCENT = 37;
const byte DEGREE  = 167;

AirQualitySensor sensor (A0);  
int quality;

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

const int WATER_PUMP = D16;
unsigned int currentTimeWater;
unsigned int lastSecondWater;

//NEOPIXEL
const int PIXELCOUNT = 12;
int brightness = 50;
int i;
int j;
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);

//WATER LEVEL ALERT TIMING
unsigned long lastWaterAlert = 0;
const unsigned long WATER_ALERT_INTERVAL = 300000; // 5 minutes in milliseconds

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

  // Print device information
  Serial.println("=== HYDROPOT DEVICE INFO ===");
  Serial.printf("Firmware Version: %s\n", System.version().c_str());
  Serial.printf("Free Memory: %lu bytes\n", System.freeMemory());
  Serial.println("============================");

  setupWiFi();
  
  mqtt.subscribe(&WaterButton);
  Time.zone(-7);

  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW);  

  // Scan for I2C devices first
  Wire.begin();
  scanI2C();

  status = bme.begin (hexAddress); 
  if (status== false){
    Serial.printf("BME280 at address 0x%02x failed to start\n", hexAddress);
    Serial.println("Could be a wiring problem, or try the other I2C address!");
    Serial.println("Check your connections and power supply to the BME280");
  }
  else {
    Serial.printf("BME280 sensor initialized successfully at address 0x%02x\n", hexAddress);
  }

  Serial.println("Waiting air quality sensor to init...");
  if (sensor.init()) {
    Serial.println("Air quality sensor ready.");
  }
  else {
      Serial.println("Air quality sensor ERROR!");
      Serial.println("Check wiring on pin A0");
  }
  
  // Give the air quality sensor some time to stabilize
  Serial.println("Air quality sensor warming up for 20 seconds...");
  delay(20000); // Wait 20 seconds for sensor to warm up
  Serial.println("Air quality sensor warm-up complete.");
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay();

  pinMode (WATER_PUMP, OUTPUT);

  //NEOPIXELS
  pixel.begin();
  pixel.setBrightness(brightness);
  pixel.show();
  pixel.clear();
  pixel.show();
}

void loop() {
  MQTT_connect();
  MQTT_ping();
 
   Adafruit_MQTT_Subscribe *subscription;
   while ((subscription = mqtt.readSubscription(1000))) {
     if (subscription == &WaterButton) {
       buttonState = atof((char *)WaterButton.lastread);
       Serial.printf("Button State: %d\n", buttonState);
 
       if (buttonState==1){
         if (waterLevelPercentage > 30) {
           Serial.printf("Remote water pump activation! Water level OK (%.1f%%)\n", waterLevelPercentage);
           digitalWrite(WATER_PUMP, HIGH);
           // Turn all pixels blue when pump is on
           for(i = 0; i < PIXELCOUNT; i++) {
             pixel.setPixelColor(i, 0x0000FF); // blue
           }
           pixel.show();
           delay(3000); 
           digitalWrite(WATER_PUMP, LOW);
           // Turn all pixels off when pump is off
           for(i = 0; i < PIXELCOUNT; i++) {
             pixel.setPixelColor(i, 0x000000); // off
           }
           pixel.show();
           Serial.printf("Remote pump OFF\n");
         }
         else {
           Serial.printf("Remote pump activation BLOCKED - water level too low (%.1f%%)\n", waterLevelPercentage);
           // Flash red to indicate blocked action
           for(int flash = 0; flash < 3; flash++) {
             for(i = 0; i < PIXELCOUNT; i++) {
               pixel.setPixelColor(i, 0xFF0000); // red
             }
             pixel.show();
             delay(300);
             for(i = 0; i < PIXELCOUNT; i++) {
               pixel.setPixelColor(i, 0x000000); // off
             }
             pixel.show();
             delay(300);
           }
         }
       }
     }
    }

    if(millis()-lastPublish > 30000) { 
      lastPublish=millis();
    if (mqtt.Update()) {
      Serial.println("Publishing sensor data...");
      TEMP.publish (tempF);
      HUMIDITY.publish (humidRH);
      MOISTURE.publish (moistureReads);
      WATERLEVEL.publish (waterLevelPercentage);
    }
  }

  dateTime = Time.timeStr ();
  timeOnly = dateTime. substring (11,19);

  tempC = bme.readTemperature();
  humidRH = bme.readHumidity();
  tempF = (tempC*9/5)+32;

  // Check if BME280 readings are valid
  if (isnan(tempC) || isnan(humidRH)) {
    Serial.println("Failed to read from BME280 sensor!");
    tempC = 0.0;
    humidRH = 0.0;
    tempF = 32.0; // Freezing point as default
  }

  int quality = sensor.slope();

  // Debug air quality sensor
  int airValue = sensor.getValue();
  Serial.printf("Air Quality Raw Value: %i, Quality Level: %i\n", airValue, quality);

  moistureReads = analogRead(soilMoist);

  sensorValue = readWaterLevelSensor();
  waterLevelPercentage = map(sensorValue, 0, 520, 0, 100);


if(millis()-lastTime>1200) {
  lastTime = millis();
  
  Serial.printf("Time is %s\n",timeOnly.c_str());
  Serial.printf("Moisture is %i\n", moistureReads);
  Serial.printf("Water Level: %i (%.1f%%)\n", sensorValue, waterLevelPercentage);
  Serial.printf("Temp: %.2f%c (%.2f%cC)\n", tempF,DEGREE, tempC,DEGREE); 
  Serial.printf("Humi: %.2f%c\n",humidRH,PERCENT);
  Serial.printf("BME280 Status: %s\n", status ? "OK" : "FAILED");
  Serial.printf("Date and Time is %s\n",dateTime.c_str());
  Serial.print("Sensor value: ");
  Serial.println(sensor.getValue());
}

if (quality == AirQualitySensor::FORCE_SIGNAL) {
  Serial.println("High pollution!");
  AIRQUALITY.publish("High pollution! ");
  for(i=0;i<PIXELCOUNT;i++){
    pixel.setPixelColor(i,0xFF0000); // red
    pixel.show();
    delay(50);
  }
  pixel.show();
  pixel.clear(); 
  pixel.show(); 
}
else if (quality == AirQualitySensor::HIGH_POLLUTION) {
  Serial.println("High pollution!");
  AIRQUALITY.publish("High pollution!");
  for(i=0;i<PIXELCOUNT;i++){
    pixel.setPixelColor(i,0xFF8000); // orange
    pixel.show();
    delay(50);
  }
  pixel.show();
  pixel.clear();  
  pixel.show();
}
else if (quality == AirQualitySensor::LOW_POLLUTION) {
  Serial.println("Low pollution!");
  AIRQUALITY.publish("Low pollution!");
  for(i=0;i<PIXELCOUNT;i++){
    pixel.setPixelColor(i,0xFFFF00); // yellow
    pixel.show();
    delay(50);
  }
  pixel.show();
  pixel.clear();
  pixel.show(); 
}
else if (quality == AirQualitySensor::FRESH_AIR) {
  Serial.println("Fresh air."); 
  AIRQUALITY.publish("Fresh air");
  for(i=0;i<PIXELCOUNT;i++){
    pixel.setPixelColor(i,0x00FF00); // green
    pixel.show();
    delay(50);
  }
  pixel.show();
  pixel.clear();
  pixel.show();
}
else {
  // Debug: Unknown air quality state
  Serial.printf("Unknown air quality state: %i (Raw value: %i)\n", quality, sensor.getValue());
} 

if (waterLevelPercentage < 30) {
  Serial.println("Low water level!");
  WATERLEVEL.publish("Low water level!");
  
  // Flash yellow 10 times every 5 minutes for low water
  if (millis() - lastWaterAlert > WATER_ALERT_INTERVAL) {
    lastWaterAlert = millis();
    for(int flash = 0; flash < 10; flash++) {
      // Turn all pixels yellow
      for(i = 0; i < PIXELCOUNT; i++) {
        pixel.setPixelColor(i, 0xFFFF00); // yellow
      }
      pixel.show();
      delay(200);
      
      // Turn all pixels off
      for(i = 0; i < PIXELCOUNT; i++) {
        pixel.setPixelColor(i, 0x000000); // off
      }
      pixel.show();
      delay(200);
    }
  }
}
else if (waterLevelPercentage > 80) {
  Serial.println("High water level!");
  
  // Flash green 5 times every 5 minutes for high water
  if (millis() - lastWaterAlert > WATER_ALERT_INTERVAL) {
    lastWaterAlert = millis();
    for(int flash = 0; flash < 5; flash++) {
      // Turn all pixels green
      for(i = 0; i < PIXELCOUNT; i++) {
        pixel.setPixelColor(i, 0x00FF00); // green
      }
      pixel.show();
      delay(200);
      
      // Turn all pixels off
      for(i = 0; i < PIXELCOUNT; i++) {
        pixel.setPixelColor(i, 0x000000); // off
      }
      pixel.show();
      delay(200);
    }
  }
}

// Only turn pump on if soil is DRY (low reading = dry soil) AND water level is sufficient
if (moistureReads < 1000 && waterLevelPercentage > 30){
    Serial.printf("Soil is dry (moisture: %i) and water level OK (%.1f%%) - turning pump ON\n", moistureReads, waterLevelPercentage);
    digitalWrite(WATER_PUMP,HIGH);
    // Turn all pixels blue when pump is on
    for(i = 0; i < PIXELCOUNT; i++) {
      pixel.setPixelColor(i, 0x0000FF); // blue
    }
    pixel.show();
    delay(500); 
    digitalWrite(WATER_PUMP,LOW);
    // Turn all pixels off when pump is off
    for(i = 0; i < PIXELCOUNT; i++) {
      pixel.setPixelColor(i, 0x000000); // off
    }
    pixel.show();
    Serial.printf("Pump OFF\n");
}
else if (moistureReads < 1000 && waterLevelPercentage <= 30) {
    Serial.printf("Soil is dry but water level too low (%.1f%%) - NOT turning pump on!\n", waterLevelPercentage);
}
else if (moistureReads >= 1000) {
    Serial.printf("Soil moisture OK (moisture: %i) - pump not needed\n", moistureReads);
}

display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0,0);
display.printf("Time: %s\n",timeOnly.c_str());
display.setCursor(0,8);
display.printf("Temp: %.1f%c Hum: %.1f%c\n",tempF,DEGREE,humidRH,PERCENT);
display.setCursor(0,24);
display.printf("Moisture: %i\n", moistureReads);
display.setCursor(0,16);

display.printf("My Hydro Flower");
display.display();

} // End of loop() function

void MQTT_connect() {
  int8_t ret;
 
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { 
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000); 
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus = true;

  if ((millis()-last)>60000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}

int readWaterLevelSensor() {
  digitalWrite(sensorPower, HIGH);
  delay(10);
  int reading = analogRead(sensorPin);
  digitalWrite(sensorPower, LOW);
  return reading;
}

void setupWiFi() {
  Serial.printf("Setting up WiFi credentials...\n");
  
  
  WiFi.clearCredentials();
  Serial.printf("Has Credentials = %i\n",WiFi.hasCredentials());
  
  // Use WiFi credentials from credentials.h
  WiFi.setCredentials(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to WiFi...\n");
  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
    delay(100);
  }
  Serial.printf("\nWiFi Connected!\n");
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
  Serial.println("WiFi setup complete - ready for MQTT!\n");
}
