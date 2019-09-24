// GSM Module - Rx- D4
//              TX- D3
// RTC        -SCL- D1
//             SDA- D2

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include<PubSubClient.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

SoftwareSerial SIM900(D3, D4);

const char *ssid = "XXXXX"; // Change to your Wifi network SSID
const char *pass = "XXXXX"; //Change to your Wifi network password

const char *mqtt_server = "m16.cloudmqtt.com";
const int mqtt_port = 17583;
const char *mqttuser = "XXXXX"; // Mqtt host username
const char *mqttpass = "XXXXX"; // mqtt password

const char *topic = "ledcontrol1";
const char *payload1 = "on";
const char *payload0 = "off";

const char *topic2 = "ledcontrol2";
const char *payload2_1 = "on";
const char *payload2_0 = "off";


int hr;
int m;

int hr2;
int m2;

int  state1 = 0;
int  state2 = 0;



#define MyApiKey "XXXXX-XXXXX-XXXXX-XXXXX-XXXXX" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard

bool isConnected = false;

void callback(String topic,byte* payload,unsigned int length1)
{ 
Serial.print("message arrived[");
Serial.print(topic);
Serial.println("]");
String msgg;

for(int i=0;i<length1;i++)
{
 Serial.print((char)payload[i]);
 msgg += (char)payload[i]; 
 
}
if(topic == "ledcontrol1")
{
 if(msgg == "onn")
 {
 relay1high();
}
else if(msgg == "off")
{
 relay1low();
}
}

if(topic == "ledcontrol2")
{
 if(msgg == "onn")
 {
 relay2high();
}
else if(msgg == "off")
{
 relay2low();
}
}

if(topic == "alarm1")
{
 hr= isdigit(msgg[1])*10 + isdigit(msgg[2]);
 m = isdigit(msgg[4])*10 + isdigit(msgg[5]);
 
}

if(topic2 == "alarm2")
{
 hr2= isdigit(msgg[1])*10 + isdigit(msgg[2]);
 m2 = isdigit(msgg[4])*10 + isdigit(msgg[5]);
 
}

}

WiFiClient espclient;
PubSubClient client1(mqtt_server,19405,callback,espclient);

void turnOn(String deviceId) {
  if (deviceId == "------------------") // Device ID of first device
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic, payload1);
  }
  else if (deviceId == "------------------") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic2, payload2_1);    
}

void turnOff(String deviceId) 
{
   if (deviceId == "-------------------------") // Device ID of first device
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     client1.publish(topic, payload0);
  
   }
   else (deviceId == "-------------------------") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic2, payload2_0);
}
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
          
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload); 
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
       
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}









String textMessage;

// Create a variable to store Lamp state
String lampState = "HIGH";

const int relay = D5;
const int relay2 = D6;


void setup() {
  
  #ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

// Initializing serial commmunication
  Serial.begin(115200); 
  SIM900.begin(38400);

if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Set relay as OUTPUT
  pinMode(relay, OUTPUT);

  pinMode(relay2, OUTPUT);

  // By default the relay is off
  digitalWrite(relay, LOW);
  digitalWrite(relay2, LOW);
  
  

  delay(15000);
  Serial.print("SIM900 ready...");

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);
  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);

  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
  delay(1000);
  Serial.println("Connecting to WiFi..");
  }
  
  Serial.println("Connected to the WiFi network");
  
  client1.setServer(mqtt_server, mqtt_port);

  webSocket.begin("iot.sinric.com", 80, "/");
  
  client1.setCallback(callback);

    // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);

  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);
 }

 void reconnect()
 {
 while (!client1.connected()) 
 {
 Serial.println("Connecting to MQTT…");
 if (client1.connect("ESP8266Client", mqttuser, mqttpass ))
 {
 Serial.println("connected");
client1.subscribe("ledcontrol1");
client1.subscribe("ledcontrol2");
 }
 else
 { Serial.print("failed with state");
 Serial.println(client1.state());
 delay(2000);
 }
 }
 }


void loop()
{

webSocket.loop();

time();
  
  if(!client1.connected())
  {
 reconnect();
 }
 client1.loop();

 
 if(SIM900.available()>0)
  {
    textMessage = SIM900.readString();
    Serial.print(textMessage);    
    delay(10);
  } 
  if(textMessage.indexOf("on1")>=0)
  {
    // Turn on relay and save current state
      client1.publish(topic, payload1);
       
  }
  if(textMessage.indexOf("off1")>=0)
  {
    // Turn off relay and save current state
    client1.publish(topic, payload0);
  }
  if(textMessage.indexOf("on2")>=0)
  {
    // Turn on relay and save current state
      client1.publish(topic2, payload2_1);
       
  }
  if(textMessage.indexOf("off2")>=0)
  {
    // Turn off relay and save current state
    client1.publish(topic2, payload2_0);
  }

  delay(500);
}



void relay1high()
{
  digitalWrite(relay, HIGH);
    Serial.println("Relay set to ON");  
    textMessage = "";
    
     state1=1;
}

void relay1low()
{
  digitalWrite(relay, LOW);
    Serial.println("Relay set to off");  
    textMessage = "";
    client1.publish(topic, payload0);
    state1=0;
}

void relay2high()
{
  digitalWrite(relay2, HIGH);
    Serial.println("Relay set to ON");  
    textMessage = "";

    state2=1;
}

void relay2low()
{
  digitalWrite(relay2, LOW);
    Serial.println("Relay set to off");  
    textMessage = "";
    state2=0;
}

void time()
{
  DateTime now = rtc.now();
  
   Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print("  ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    alarmrelay1();
    alarmrelay2();
}

void alarmrelay1()
{
  DateTime now = rtc.now();
  if(hr==now.hour())
  {
    if(m==now.minute())
    {
      switch(state1)
      {
      case 0:
      {
        client1.publish(topic, payload1);
      }
      break;
      case 1:
      {
        client1.publish(topic, payload0);
      }
      
    
  }
}
}
}


void alarmrelay2()
{
  DateTime now = rtc.now();
  if(hr2==now.hour())
  {
    if(m2==now.minute())
    {
      switch(state1)
      {
      case 0:
      {
        client1.publish(topic2, payload2_1);
      }
      break;
      case 1:
      {
        client1.publish(topic2, payload2_0);
      }
      
    
  }
}
}
}
 
