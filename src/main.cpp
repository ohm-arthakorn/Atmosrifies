#define RelayPin 27

#include <WiFi.h>
#include <SoftwareSerial.h>
#include "Adafruit_SPIDevice.h"
#include <Wire.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

SoftwareSerial mySerial(33, 32); // RX, TX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;
WiFiClient espClient;
PubSubClient client(espClient);

// WiFi Credentials
const char *ssid = "daw";
const char *password = "123456789";
const char *mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char *mqtt_Client = "Art_proj";
const char *mqtt_username = "";
const char *mqtt_password = "";
const char *mqtt_topic = "air_fill/val";
const char *mqtt_sub_topic = "air_control"; 
// I2C Scanner
void checkingi2c()
{
  Wire.begin();
  Serial.println("I2C Scanner");
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0)
    {
      Serial.print("Found address: ");
      Serial.print(i, DEC);
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.println();
  Serial.println("I2C scanner. Scanning ...");
  byte count = 0;

  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println("Recieved : " + message);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }


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

// Function to Read and Send PM Data to Blynk
void ReadAndChangePMData()
{
  int index = 0;
  char value;
  char previousValue;

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
  while (mySerial.available()) mySerial.read();

  // Print Values
  Serial.printf("{ \"pm1\": %d ug/m3, \"pm2_5\": %d ug/m3, \"pm10\": %d ug/m3 }\n", pm1, pm2_5, pm10);

  delay(1000);

  // if pm2.5 is greater than 38, turn on the relay
  if (pm2_5 > 38)
  {
    digitalWrite(RelayPin, 1);
  }
  else
  {
    digitalWrite(RelayPin, 0);
  }
}

void setup()
{
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, HIGH); // make sure Relay started with closed state !
  Wire.begin();

  Serial.begin(115200);
  mySerial.begin(9600);

  // connect WiFi
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("Connected to MQTT Broker!");
    }
    else
    {
      Serial.print("Failed with state: ");
      Serial.print(client.state());
      Serial.println(" Trying again...");
      delay(1000);
    }
  }

  // Connect to Blynk
  checkingi2c();
}

// function that can control a foggy from blynk app
void loop()
{
  ReadAndChangePMData();
  if (!client.connected())
  {
    Serial.println("Reconnecting to MQTT...");
    client.connect("ESP32Client");
  }
  

  // ส่งค่า JSON ผ่าน MQTT
  JsonDocument doc;

  doc["temperature"] = random(20, 30); // Simulated temperature data
  doc["humidity"] = random(50, 80);

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  client.publish(mqtt_topic, jsonBuffer);
  Serial.println("Sent: " + String(jsonBuffer));

  delay(1000); // ส่งทุก 5 วินาที
  client.loop();
}
