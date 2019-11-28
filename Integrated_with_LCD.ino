// GSM Module - Rx- D4
//              TX- D3
// RTC        - SCL- D1
//              SDA- D2
// 16X02 LCD  - SCL- D1
//              SDA- D2

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include<PubSubClient.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

RTC_DS3231 rtc;

LiquidCrystal_I2C lcd(0x3f, 16, 2);//change the 'ox3f' part acc to your device's configuration

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

SoftwareSerial SIM900(D3, D4); // Tx, Rx

const char *ssid = "XXXXXX"; // cannot be longer than 32 characters!
const char *pass = "XXXXXX";

const char *mqtt_server = "m16.cloudmqtt.com";
const int mqtt_port = 17583;
const char *mqttuser = "XXXXXXX";
const char *mqttpass = "XXXXXXX";

const char *topic = "ledcontrol1";
const char *payload1 = "onn";
const char *payload0 = "off";

const char *topic2 = "ledcontrol2";
const char *payload2_1 = "onn";
const char *payload2_0 = "off";


int hr;
int m;

int hr2;
int m2;

int  state1 = 0;
int  state2 = 0;



#define MyApiKey "XXXXXX-XXXX-XXXX-XXXX-XXXX-XXXX" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard

bool isConnected = false;

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
      lcd.setCursor(6,0);
      lcd.print("V-1");
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
 print(1,1,1);
}
else if(msgg == "off")
{
 relay1low();
 print(1,0,1);
}
}

if(topic == "ledcontrol2")
{
 if(msgg == "onn")
 {
 relay2high();
 print(2,1,1);
}
else if(msgg == "off")
{
 relay2low();
 print(2,0,1);
}
}

if(topic == "alarm1")
{
 hr= isdigit(msgg[1])*10 + isdigit(msgg[2]);
 m = isdigit(msgg[4])*10 + isdigit(msgg[5]);
 Serial.println("integer value");
 Serial.print(hr);
 Serial.print(m);
 
}

if(topic == "alarm2")
{
 hr2= isdigit(msgg[1])*10 + isdigit(msgg[2]);
 m2 = isdigit(msgg[4])*10 + isdigit(msgg[5]);

 Serial.print(hr2+":"+m2);
 
}

}

WiFiClient espclient;
PubSubClient client1(mqtt_server,19405,callback,espclient);

String textMessage;

const int relay = D5;
const int relay2 = D6;




void setup() {

  
  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  lcd.print("W-0T-0V-0");
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
  lcd.setCursor(3,0);
  lcd.print("T-1");

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
client1.subscribe("alarm1");
client1.subscribe("alarm2");

lcd.setCursor(0,0);
lcd.print("W-1");

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
alarmrelay1();
alarmrelay2();
  
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
       print(1,1,2);
  }
  if(textMessage.indexOf("off1")>=0)
  {
    // Turn off relay and save current state
    client1.publish(topic, payload0);
    print(1,0,2);
  }
  if(textMessage.indexOf("on2")>=0)
  {
    // Turn on relay and save current state
      client1.publish(topic2, payload2_1);

      print(2,1,2);
       
  }
  if(textMessage.indexOf("off2")>=0)
  {
    // Turn off relay and save current state
    client1.publish(topic2, payload2_0);
    print(2,0,2);
  }

  if(textMessage.indexOf("alarm1")>=0)
{
 hr= isdigit(textMessage[8])*10 + isdigit(textMessage[9]);
 m = isdigit(textMessage[11])*10 + isdigit(textMessage[12]);
 
}

if(topic2 == "alarm2")
{
 hr2= isdigit(textMessage[8])*10 + isdigit(textMessage[9]);
 m2 = isdigit(textMessage[11])*10 + isdigit(textMessage[12]);
 
}

  delay(500);
}



void turnOn(String deviceId) {
  if (deviceId == "_________________________") // Device ID of first device
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic, payload1);
    print(1,1,3);
  }
  else if (deviceId == "__________________________") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic2, payload2_1);

    print(2,1,3);
}

}

void turnOff(String deviceId) 
{
   if (deviceId == "_______________________________") // Device ID of first device
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     client1.publish(topic, payload0);

     print(1,0,3);
   }
   else if (deviceId == "_________________________________") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    client1.publish(topic2, payload2_0);

    print(2,0,3);
}
}


void relay1high()
{
  digitalWrite(relay, HIGH);
  textMessage = "";
  state1=1;
     
    Serial.println("Relay set to ON");  
    
}

void relay1low()
{
  digitalWrite(relay, LOW);
  textMessage = "";
    state1=0;
    
    Serial.println("Relay set to off");  
    
}

void relay2high()
{
  digitalWrite(relay2, HIGH);
  textMessage = "";

    state2=1;
    
    Serial.println("Relay set to ON");  
    
}

void relay2low()
{
  digitalWrite(relay2, LOW);
  textMessage = "";
    state2=0;
    
    Serial.println("Relay set to off");  
    
}

void time()
{
  DateTime now = rtc.now();
  lcd.setCursor(11,0);

  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute());
  
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

}

void alarmrelay1()
{
  DateTime now = rtc.now();
  if(hr==now.hour())
  {
    if(m==now.minute())
    {
      Serial.print("alarm1 activated");
      switch(state1)
      {
      case 0:
      {
        client1.publish(topic, payload1);
        hr = 100;
        m  = 100;
      }
      break;
      case 1:
      {
        client1.publish(topic, payload0);

        hr = 100;
        m  = 100;
      }
      break;
      
    
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
      Serial.print("alarm2 activated");
      switch(state1)
      {
      case 0:
      {
        client1.publish(topic2, payload2_1);
        hr2 = 100;
        m2  = 100;
      }
      break;
      case 1:
      {
        client1.publish(topic2, payload2_0);
        hr2 = 100;
        m2  = 100;
      }
      break;
      
    
  }
}
}
}

 void print(int m, int n, int o)
 {
  
  lcd.setCursor(0,1);
  
  switch (m)
  {
    case 1:
    {
      switch (n)
      {
        case 1:
        {
          switch (o)
          {
            case 1:
            lcd.print("Bulb On MQTT    ");
            break;

            case 2:
            lcd.print("Bulb On Text    ");
            break;

            case 3:
            lcd.print("Bulb On Alexa    ");
            break;
          }
        }
        break;

        case 0:
        {
          switch (o)
          {
          case 1:
            lcd.print("Bulb Off MQTT    ");
            break;

            case 2:
            lcd.print("Bulb Off Text    ");
            break;

            case 3:
            lcd.print("Bulb Off Alexa    ");
            break;
          }
          break;
        }
        break;
    }
    }
    break;

        case 2:
        {
          switch (n)
      {
        case 1:
        {
          switch (o)
          {
            case 1:
            lcd.print("Socket On MQTT    ");
            break;

            case 2:
            lcd.print("Socket On Text    ");
            break;

            case 3:
            lcd.print("Socket On Alexa    ");
            break;
          }
        }
        break;

        case 0:
        {
          switch (o)
          {
                  case 1:
            lcd.print("Socket Off MQTT    ");
            break;

            case 2:
            lcd.print("Socket Off Text    ");
            break;

            case 3:
            lcd.print("Socket Off Alexa    ");
            break;
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  }
 }
