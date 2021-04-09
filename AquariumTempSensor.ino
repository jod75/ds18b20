/*  
 *  The aim of this program is to read temperature data from DS18B20 sensors and 
 *  upload this data to https://www.thingspeak.com/ by using ESP8266 microcontroller.
 *   
 *  For information about DS18B20 refer to https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 *  
 *  In order to upload data to ThingSpeak, you need to create a free account in the said service. Once
 *  you create an account, you need to create a channel and obtain a Write API key and Channel ID.
 *  
 *  The code below builds on the Tester.ino example by Dallas Temperature sensor library:
 *     https://github.com/milesburton/Arduino-Temperature-Control-Library 
 *  
 *  Libraries needed to compile this code are
 *  DallasTemperature.h - Dallas Temperature sensor  library by Miles Burton
 *  OneWire.h - Library for Dallas/Maxim 1-Wire Chips
 *  ESP8266Wifi.h - Built in to ESP8266/Arduino integration.
 *  ThingSpeak.h - Offical ThinkSpeak library by Mathworks:
 *      https://github.com/mathworks/thingspeak-arduino
 *  
 */

#include <DallasTemperature.h>
#include <OneWire.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>

/***************************
 * DS18B20 Settings
 **************************/
#define ONE_WIRE_BUS D2          // Digital I/O pin to which the DS18B20 is connected to.  This will act as our data bus.
#define TEMPERATURE_PRECISION 11 // Set sensor precision to 11-bits.  Check the manufacturer's specifications for supported values.

/***************************
 * WIFI Settings
 **************************/
const char *ssid = "YOUR WIFI NETWORK SSID";    // SSID of wireless network
const char *password = "YOUR WIFI PASSWORD";    // Password for wireless network

/***************************
 * ThingSpeak Settings
 **************************/ 
unsigned long channelNumber = 9999999;          // Thingspeak channel ID
const char *writeAPIKey = "WRITE API KEY";      // Write API key
int fieldStart = 1;                             // The field number to where we shall start the data upload to the channel
int uploadIntervalSeconds = 60;                 // Data reading interval

/***************************
 * Serial Monitor Settings
 **************************/ 
const int baudrate = 115200;

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberOfDevices;                           // Number of temperature devices found
DeviceAddress tempDeviceAddress;               // We'll use this variable to store a found device address

void setup()
{
  Serial.begin(baudrate);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to WiFi with ssid: " + String(ssid));  

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: " + WiFi.localIP());  

  ThingSpeak.begin(wifiClient);
  sensors.begin();

  // Get a count of devices on the bus
  Serial.println("Locating devices...");
  numberOfDevices = sensors.getDeviceCount();
  Serial.print("Found " + String(numberOfDevices, DEC) + " devices.");
  
  // Report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode())
    Serial.println("ON");
  else
    Serial.println("OFF");

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device at " + String(i, DEC) + " with address: ");      
      printAddress(tempDeviceAddress);
      Serial.println();

      Serial.println("Setting resolution to " + String(TEMPERATURE_PRECISION, DEC));      

      // Set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
      Serial.println();
    }
    else
    {
      Serial.println("Found ghost device at " + String(i, DEC) + " but could not detect address. Check power and cabling");            
    }
  }
}

// Function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");

    Serial.print(deviceAddress[i], HEX);
  }
}

void loop()
{
  Serial.println("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("Done");

  // Iterate through each sensor and send readings to ThinkSpeak
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))
    {
      // Change this line to use getTempFByIndex if you need to read your temperateure in Fahrenheit
      float temp = sensors.getTempCByIndex(i);
      ThingSpeak.setField(i + fieldStart, temp);
      Serial.println("Sensor #" + String(i, DEC));
      Serial.println("Temperature: " + String(temp) + "°C");      
    }
    else
    {
      Serial.println("Found ghost device at " + String(i, DEC) + " but could not detect address. Check power and cabling");      
    }
  }

  Serial.println("Uploading data to thingspeak ");
  ThingSpeak.writeFields(channelNumber, writeAPIKey); 
    
  delay(uploadIntervalSeconds * 1000);
}
