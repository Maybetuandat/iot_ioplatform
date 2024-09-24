#include <Arduino.h>
#include <WiFi.h>
#include<PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include<DHT.h>
#include <cmath>
#define ledGPIO 13 
#define fanGPIO 12  
#define airConditioner 14 
#define dht11GPIO 5 
#define DHTTYPE DHT11
#define lightGPIO 34
const char* sensorData = "home/sensor/data";  
const char* statusLedRequest = "home/led/request";
const char*  statusFanRequest =  "home/fan/request";
const char* statusAirConditionerRequest =  "home/air_conditioner/request";
const char* statusLedResponse = "home/led/response";
const char*  statusFanResponse =  "home/fan/response";
const char* statusAirConditionerResponse =  "home/air_conditioner/response";
DHT dht(dht11GPIO, DHTTYPE);
const char *ssid = "NhatroT4"; // wifi name
const char* password = "ngocmai284"; //wifi password
const char* mqtt_server = "192.168.2.7";
const  int mqtt_port = 1883;
const char* mqtt_username = "maybetuandat";
const char* mqtt_password = "1";
const unsigned long sendInterval = 5000;
unsigned long lastSend = 0;
WiFiClient espClient;
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
      client.subscribe(statusLedRequest);
      client.subscribe(statusFanRequest);
      client.subscribe(statusAirConditionerRequest);
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
    float factor = powf(10.0f, decimalPlaces);  
    return roundf(value * factor) / factor;     
}
void publich_message_data(const char* topic)
{
    StaticJsonDocument<256> jsonDoc;   
    float temperature = roundTo(dht.readTemperature(),2);
    jsonDoc["temperature"] = temperature;
    jsonDoc["humidity"] = dht.readHumidity();
    if(isnan(dht.readTemperature()) || isnan(dht.readHumidity()))
    {
      Serial.println("Failed to read from DHT sensor");
      return;
    }
    int analogValue = 4095 - analogRead(lightGPIO);
    jsonDoc["light_level"] = analogValue;
    char buffer[256];
    serializeJson(jsonDoc, buffer);  
    Serial.println(buffer);
    client.publish(topic, buffer);
} 
void call_back(char* topic, byte* payload, unsigned int length)
{
    String incoming_message = "";
    for(int i=0; i<length; i++) incoming_message += (char)payload[i]; 
     Serial.print(topic);
     Serial.print(" ");           
     Serial.println(incoming_message);
    if(strcmp(topic,statusLedRequest)  == 0)
    { 
      if(incoming_message == "1")
      {
            digitalWrite(ledGPIO, HIGH);
            client.publish(statusLedResponse, "1");
            Serial.println("led on");
      }  
      else
      {
        digitalWrite(ledGPIO, LOW);
        client.publish(statusLedResponse, "0");
      }
        
    }
    if(strcmp(topic,statusFanRequest) == 0)
    {
      if(strcmp(incoming_message.c_str(), "1") == 0)
      {
            digitalWrite(fanGPIO, HIGH);
             client.publish(statusFanResponse, "1");
      }
        
      else
      {
         digitalWrite(fanGPIO, LOW);
        client.publish(statusFanResponse, "0");
      }    
    }
    if(strcmp(topic,statusAirConditionerRequest) == 0)
    {
      if(strcmp(incoming_message.c_str(), "1") == 0)
      {
            digitalWrite(airConditioner, HIGH);
            client.publish(statusAirConditionerResponse, "1");
      }
      else 
      {
            digitalWrite(airConditioner, LOW);
            client.publish(statusAirConditionerResponse, "0");
      }
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

