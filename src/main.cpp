#define BLYNK_TEMPLATE_ID "TMPL6CJvPqOYw"
#define BLYNK_TEMPLATE_NAME "Air Filtering"
#define BLYNK_AUTH_TOKEN "ov9pT-x8NgNHIvmLpBkkP4qh0kUNCjEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SoftwareSerial.h>
#include "Adafruit_SPIDevice.h"

SoftwareSerial mySerial(13, 12); // RX, TX
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

// WiFi Credentials
const char *ssid = "Ohm";
const char *password = "123456789";

void setup()
{
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

  // Connect to Blynk
  Blynk.begin("ov9pT-x8NgNHIvmLpBkkP4qh0kUNCjEN", "Ohm", "123456789");
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

void loop()
{
  Blynk.run();
  readPMData();
}
