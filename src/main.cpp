#define RelayPin 27
// ประกาศ Subscribe และ Publish ที่จะใช้ในการส่งข้อมูลเข้า Node Red
#define Subscribe "data/sub"
#define Publish "data/pub"

#include <WiFi.h>
#include <SoftwareSerial.h>
#include "Adafruit_SPIDevice.h"
#include <Wire.h>
#include <PubSubClient.h>

SoftwareSerial mySerial(33, 32); // RX, TX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

// * ส่วนจัดการกับการเชื่อมต่อต่าง ๆ ไม่ว่าจะเป็น WiFi หรือ MQTT

// WiFi
const char *ssid = "Ohm";
const char *password = "123456789";

// MQTT Server
WiFiClient espClient;
PubSubClient client(espClient);

const int mqttPort = 1883;
const char *mqttServer = "broker.mqtt.cool";
const char *mqttClient = "Arthakorn";
const char *mqttUsername = "";
const char *mqttPassword = "";

// จัดการส่งข้อมูลเข้า Node Red
String DataString;
long lastMsg = 0;
int value = 0;
char msg[100];

// ฟังก์ชันสำหรับการเชื่อมต่อ MQTT เข้ากับ Node Red
void reconnect(){
  while (!client.connected()){
    Serial.print("Attempting MQTT connection...");
    if(client.connect(mqttClient, mqttUsername, mqttPassword)){
      Serial.println("Connected !");
      client.subscribe("sub");
    }
    else{
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println("Try again in 5 seconds !");
      delay(5000);
    }
  }
}

// ฟังก์ชัน Callback สำหรับการรับข้อมูลจาก Node Red
void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;

  for(int i = 0; i < length; i++){
    message = message + char(payload[i]);
  }

  Serial.println(message);
}


// I2C Scanner
void checkingi2c()
{
  while (!Serial)
  {
  }

  Serial.println();
  Serial.println("I2C scanner. Scanning ...");
  byte count = 0;

  Wire.begin();
  for (byte i = 8; i < 120; i++) // Loop ค้นหา Address
  {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0)
    {
      Serial.print("Found address: ");
      Serial.print(i, DEC);
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      delay(1);
    }
  }
  Serial.println("Done.");
  Serial.print("Found ");
  Serial.print(count, DEC);
  Serial.println(" device(s).");
 
}

// * ฟังก์ชันสำหรับการอ่านค่า PM 2.5 และ PM 10 จากเซนเซอร์ อีกทั้งยังส่งข้อมูลไปยัง Node Red 
void ReadandSendPM()
{
  int index = 0;
  char value;
  char previousValue;
  unsigned long initialMillis = millis();

  while (mySerial.available())
  {
    value = mySerial.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4D))
    {
      Serial.println("Cannot find the data header.");
      break;
    }

    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14)
    {
      previousValue = value;
    }
    else if (index == 5)
    {
      pm1 = 256 * previousValue + value;
    }
    else if (index == 7)
    {
      pm2_5 = 256 * previousValue + value;
    }
    else if (index == 9)
    {
      pm10 = 256 * previousValue + value;
    }
    else if (index > 15)
    {
      break;
    }
    index++;
  }

  // Clear Serial Buffer
  while (mySerial.available())
    mySerial.read();

  // Print Values
  Serial.printf("{ \"pm1\": %d ug/m3, \"pm2_5\": %d ug/m3, \"pm10\": %d ug/m3 }\n", pm1, pm2_5, pm10);


  delay(1000);
  
  // * ถ้า pm2.5 มีค่าเกิน 38 ug/m^3 จะทำการพ่นไอน้ำและส่งแจ้งเตือนผ่าน Node Red
  if(pm2_5 > 38){
    digitalWrite(RelayPin, 1);
  }
  else{
    digitalWrite(RelayPin, 0);
  }

  // * ส่งข้อมูลไปยัง Node Red
  if(initialMillis - lastMsg > 1000){
    lastMsg = initialMillis;
    ++value;
    DataString = "{ \"pm1\": " + String(pm1) + ", \"pm2_5\": " + String(pm2_5) + ", \"pm10\": " + String(pm10) + " }";
    DataString.toCharArray(msg, 100);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("sub",msg);
  }

  delay(1);
  client.loop();
}


// * ฟังก์ชันสำหรับการเชื่อมต่อ WiFi
void connectedWiFi(){
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, HIGH); // make sure Relay started with closed state ! 
  Wire.begin();

  Serial.begin(115200);
  mySerial.begin(9600);

  // * ส่วนที่เกี่ยวกับการเชื่อมต่อ MQTT 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.subscribe(Subscribe);

  // * ประกาศเรียกใช้ฟังก์ชันสำหรับการเชื่อมต่อ WiFi
  connectedWiFi();
  checkingi2c();
}

// function that can control a foggy from blynk app
void loop()
{
  ReadandSendPM();
}