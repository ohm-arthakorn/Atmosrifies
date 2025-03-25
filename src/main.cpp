#define RelayPin 27

#include <WiFi.h>
#include <SoftwareSerial.h>
#include "Adafruit_SPIDevice.h"
#include <Wire.h>

SoftwareSerial mySerial(33, 32); // RX, TX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

// WiFi Credentials
const char *ssid = "dawdarari";
const char *password = "201048840102";
const char *mosquitto_server = 1883;
const int mosquitto_port = 1883;
const char *mosquitto_Client = "Apitsara";
const char *mosquitto_username = "";
const char *mosquitto_password = "";

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
  while (mySerial.available())
    mySerial.read();

  // Print Values
  Serial.printf("{ \"pm1\": %d ug/m3, \"pm2_5\": %d ug/m3, \"pm10\": %d ug/m3 }\n", pm1, pm2_5, pm10);


  delay(1000);
  
  // if pm2.5 is greater than 38, turn on the relay 
  if(pm2_5 > 38){
    digitalWrite(RelayPin, 1);
  }
  else{
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
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Client.publish("Apitsara",msg);
  
  // Connect to Blynk
  checkingi2c();
}

// function that can control a foggy from blynk app
void loop()
{
  ReadAndChangePMData();
}