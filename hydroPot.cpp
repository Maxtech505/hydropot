/* 
 * Project My hydropot
 * Author: Maximo Regalado
 * Date: 2025-07-17
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
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
#include "Air_Quality_Sensor.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"
#include "Colors.h"

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
const int hexAddress = 0x76;
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

const int PIXELCOUNT = 15;
int brightness = 50;
int i;
int j;
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

  Serial.printf("Connecting to Internet \n");
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n Connected!!!!!! \n");
  mqtt.subscribe(&WaterButton);
  Time.zone(-7);
  Particle.syncTime();

  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW);  

  status = bme.begin (hexAddress); 
  if (status== false){
    Serial.printf("BME280 at address 0x%02x failed to start", hexAddress);
  }

  Serial.println("Waiting sensor to init...");
  if (sensor.init()) {
    Serial.println("Sensor ready.");
  }
  else {
      Serial.println("Sensor ERROR!");
  }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay();

  pinMode (WATER_PUMP, OUTPUT);

  pixel.begin ();
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
         Serial.printf("Remote water pump activation!\n");
         digitalWrite(WATER_PUMP, HIGH);
         pixel.setPixelColor(0,blue);
         pixel.show();
         delay(3000); 
         digitalWrite(WATER_PUMP, LOW);
         Serial.printf("Remote pump OFF\n");
         pixel.setPixelColor(0,black);
         pixel.show();
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

  int quality = sensor.slope();

  moistureReads = analogRead(soilMoist);

  sensorValue = readWaterLevelSensor();
  waterLevelPercentage = map(sensorValue, 0, 520, 0, 100);


if(millis()-lastTime>1200) {
  lastTime = millis();
  
  Serial.printf("Time is %s\n",timeOnly.c_str());
  Serial.printf("Moisture is %i\n", moistureReads);
  Serial.printf("Water Level: %i (%.1f%%)\n", sensorValue, waterLevelPercentage);
  Serial.printf("Temp: %.2f%c\n ", tempF,DEGREE); 
  Serial.printf("Humi: %.2f%c\n",humidRH,PERCENT);
  Serial.printf("Date and Time is %s\n",dateTime.c_str());
  Serial.print("Sensor value: ");
  Serial.println(sensor.getValue());
}

if (quality == AirQualitySensor::FORCE_SIGNAL) {
  Serial.println("High pollution!");
  AIRQUALITY.publish("High pollution! ");
  for(i=0;i<60;i++){
    pixel.setPixelColor(i,red);
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
  for(i=0;i<60;i++){
    pixel.setPixelColor(i,orange);
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
  for(i=0;i<60;i++){
    pixel.setPixelColor(i,yellow);
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
  for(i=0;i<60;i++){
    pixel.setPixelColor(i,green);
    pixel.show();
    delay(50);
}
pixel.show();
pixel.clear();
pixel.show();
} 

if (waterLevelPercentage < 30) {
  Serial.println("Low water level!");
  WATERLEVEL.publish("Low water level!");
  
  for(int flash=0; flash<3; flash++) {
    for(i=0;i<PIXELCOUNT;i++){
      pixel.setPixelColor(i,yellow);
    }
    pixel.show();
    delay(300);
    
    pixel.clear();
    pixel.show();
    delay(300);
  }
}

if (moistureReads>3000){
    digitalWrite(WATER_PUMP,HIGH);
    pixel.setPixelColor(60,blue);
    pixel.show();
    Serial.printf("Pump ON\n");
    delay(500); 
    digitalWrite(WATER_PUMP,LOW);
    Serial.printf("Pump OFF:\n");
      pixel.setPixelColor(60,black);
      pixel.show();
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
