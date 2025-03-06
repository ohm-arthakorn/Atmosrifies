#define BLYNK_TEMPLATE_ID "TMPL6CJvPqOYw"
#define BLYNK_TEMPLATE_NAME "Air Filtering"
#define BLYNK_AUTH_TOKEN "ov9pT-x8NgNHIvmLpBkkP4qh0kUNCjEN"
#define LED 27


#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SoftwareSerial.h>
#include "Adafruit_SPIDevice.h"
#include <Wire.h>
#include <Adafruit_CCS811.h>

Adafruit_CCS811 ccs;
SoftwareSerial mySerial(32, 33); // RX, TX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;
int eCO2;
int TVOC;

// WiFi Credentials
const char *ssid = "Ohm";
const char *password = "123456789";

// I2C Scanner
void checkingi2c()
{
  while (!Serial)
  {
  }

  Serial.println();
  Serial.println("www.9arduino.com ...");
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


void setup()
{
  pinMode(LED, OUTPUT);

  Wire.begin();
  if (!ccs.begin())
  {
    Serial.println("Failed to start sensor! Please check your wiring.");
    while (1);
  }
  // Wait for the sensor to be ready
  while (!ccs.available());

  Serial.begin(9600);
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

  // Connect to Blynk
  Blynk.begin("ov9pT-x8NgNHIvmLpBkkP4qh0kUNCjEN", "Ohm", "123456789");

  checkingi2c();
}

// Function to Read and Send PM Data to Blynk
void readPMData()
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

  // Send to Blynk Dashboard (Assign Virtual Pins)
  Blynk.virtualWrite(V1, pm1);
  Blynk.virtualWrite(V2, pm2_5);
  Blynk.virtualWrite(V3, pm10);
  

  delay(1000);
}

// Function to Read and Send eCO2 and TVOC Data to Serial Monitor
void readeCO2(){
 

  if (ccs.available())
  {
    if (!ccs.readData())
    {
      // Read eCO2 and TVOC
      eCO2 = ccs.geteCO2();
      TVOC = ccs.getTVOC();  
      Serial.print("eCO2: ");
      Serial.print(eCO2);
      Serial.print(" ppm, TVOC: ");
      Serial.print(TVOC);
      Serial.println(" ppb");
    }
    else
    {
      Serial.println("Error reading sensor data");
    }
  }
  delay(1000);


  // Send to Blynk Dashboard (Assign Virtual Pins)
  Blynk.virtualWrite(V4, eCO2);
}

// Function to Alert When High CO2 Detected
void AlertWhenHighCO2(){
  eCO2 = ccs.geteCO2();

  if(eCO2 > 1000){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
  }
}

void loop(){
  Blynk.run();
  readPMData();
  readeCO2();
  AlertWhenHighCO2();
}