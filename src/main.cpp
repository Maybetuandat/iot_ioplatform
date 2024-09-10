#include <Arduino.h>
#include <WiFi.h>
#include<PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include<DHT.h>
#include <cmath>

#define ledGPIO 13  //define for light
#define fanGPIO 12  //define for fan
#define airConditioner 14 //define for air conditioner
#define dht11GPIO 5 // define for humidity and temperature
#define DHTTYPE DHT11
#define lightGPIO 34 // su dung de do anh sang 
const char* sensorData = "home/sensor";  // tra ve ten topic va du lieu la 0 or 1, 1 la on , 0 la off
const char* statusLight = "home/light";
const char*  statusFan =  "home/fan";
const char* statusAirConditioner =  "home/air_conditioner";

DHT dht(dht11GPIO, DHTTYPE);


const char *ssid = "NhatroT4"; // wifi name
const char* password = "ngocmai284"; //wifi password


// config server
const char* mqtt_server = "c95f11febd0e4f36a313eb8f5b4dd763.s1.eu.hivemq.cloud";
const  int mqtt_port = 8883;
const char* mqtt_username = "maybetuandat";
const char* mqtt_password = "123456789aA@";


// set time use for send data to hivemmq cloud 1 minute

const unsigned long sendInterval = 6000;
unsigned long lastSend = 0;
WiFiClientSecure espClient;
PubSubClient client(espClient);


void connect_mqtt_broker()
{
  while(!client.connected())
  {
    Serial.println("Connecting to MQTT Broker . . . . . . . .. . . . . . .. . . ");
    String clientID =  "ESPClient-";
    clientID += String(random(0xffff),HEX);
    if(client.connect(clientID.c_str(),  mqtt_username, mqtt_password))
    {
      Serial.println("Connected to MQTT Broker");
      client.subscribe(statusLight);
      client.subscribe(statusFan);
      client.subscribe(statusAirConditioner);
    }
    else
    {
      Serial.print("Failed with state: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
float roundTo(float value, int decimalPlaces) {
    float factor = pow(10.0, decimalPlaces);
    return round(value * factor) / factor;
}
void publich_message_data(const char* topic)
{
    StaticJsonDocument<256> jsonDoc;   // light -> fan -> air conditioner  //0 la off , 1 là on 
    jsonDoc["light"] = digitalRead(ledGPIO);
    jsonDoc["fan"] = digitalRead(fanGPIO);
    jsonDoc["air_conditioner"] = digitalRead(airConditioner);
    jsonDoc["temperature"] = roundTo(dht.readTemperature(),2);
    jsonDoc["humidity"] = roundTo(dht.readHumidity(), 2);
    if(isnan(dht.readTemperature()) || isnan(dht.readHumidity()))
    {
      Serial.println("Failed to read from DHT sensor");
      return;
    }
    int analogValue = analogRead(lightGPIO);
     float voltage = (analogValue / 4095.0) * 3.3;
    jsonDoc["light_level"] = voltage;
    char buffer[256];
    serializeJson(jsonDoc, buffer);  
    Serial.println(buffer);
    client.publish(topic, buffer);
} 
void public_message_status_device(const char* topic, const char* message)
{
    client.publish(topic, message);  //  sử dụng để trả lại trạng thái khi click nút   sẽ thêm sau 
}
void call_back(char* topic, byte* payload, unsigned int length)
{
    String incoming_message = "";
    for(int i=0; i<length; i++) incoming_message += (char)payload[i];  // call_back cần sửa thêm 
     Serial.print(topic);
    
     Serial.print(" ");             // In ra một khoảng trắng để phân cách
     Serial.println(incoming_message); // In ra giá trị của incoming_message và xuống dòng

    if(strcmp(topic,statusLight)  == 0)
    { 
      // Serial.println("Light");
      if(incoming_message == "1")
        digitalWrite(ledGPIO, HIGH);
        
      else if(incoming_message == "0")
        digitalWrite(ledGPIO, LOW);
    }
    if(strcmp(topic,statusFan) == 0)
    {
      // Serial.println("fan");
      if(incoming_message == "1")
        digitalWrite(fanGPIO, HIGH);
      else if(incoming_message == "0")
        digitalWrite(fanGPIO, LOW);
        
      
    }

    if(strcmp(topic,statusAirConditioner) == 0)
    {
      // Serial.println("smart ac");
      if(incoming_message == "1")
        digitalWrite(airConditioner, HIGH);
      else if(incoming_message == "0")
        digitalWrite(airConditioner, LOW);
    }
   

}
void set_up_wifi()
{
  Serial.begin(9600);
  delay(1000);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to wifi...");
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
   Serial.println("\nConnected to the WiFi network");
   Serial.println(WiFi.localIP());

}
void setup_led()
{
    pinMode(ledGPIO, OUTPUT);
    pinMode(fanGPIO, OUTPUT);
    pinMode(airConditioner, OUTPUT);
}
void setup() {
    set_up_wifi();
    setup_led();
    espClient.setInsecure();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(call_back);
}

void loop() {
    if(!client.connected())
    {
      connect_mqtt_broker();

    }
    client.loop();
    
 
   unsigned long currentMillis = millis();
   if(currentMillis - lastSend >= sendInterval)
   {
     lastSend = currentMillis;
     publich_message_data(sensorData);
   }
}

