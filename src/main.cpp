#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHA.h>

// Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BROKER_ADDR IPAddress(192, 168, 1, 222)

#define LED_PIN 2
#define BROKER_ADDR IPAddress(192, 168, 1, 222)
#define WIFI_SSID "Xiaomi"
#define WIFI_PASSWORD "926810D20D"

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

HASensorNumber airidx("AirIndex", HASensorNumber::PrecisionP0);
HASensorNumber airtvoc("AirTVOC", HASensorNumber::PrecisionP0);
HASensorNumber aireco2("AirECO2", HASensorNumber::PrecisionP0);
// HANumber refreshTime("Refresh Timer");
HASelect selectv2("mySelect");
HANumber number("myNumber");

int lastidx = 0;
int lasttvoc = 0;
int lasteco2 = 0;
// ############### GAS Sens ###############

#include "SparkFun_ENS160.h"

SparkFun_ENS160 myENS;

bool printedCompensation = false;
int ensStatus;

float rh;
float tempC;


int aqi = 0;
int Tvoc = 0;
int ECO2 = 0;
// ################ AHT2X #################

#include <AHTxx.h>

float ahtValue; // to store T/RH result

AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR); // sensor address, sensor type

// ############### OLED ###################
// OLED pins
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// ########################################


unsigned long delayInterval = 10000;  // Delay interval in milliseconds
unsigned long previousMillis = 0;         // Store the last time the delay occurred


void oled_display(String text, int x, int y, int size, bool cls)
{

  if (cls)
  {
    display.clearDisplay();
  }

  display.setTextColor(WHITE);
  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
  display.display();
}




void onSelectCommand(int8_t index, HASelect* sender)
{
    switch (index) {
    case 0:
        // Option "Low" was selected
        break;

    case 1:
        // Option "Medium" was selected
        break;

    case 2:
        // Option "High" was selected
        break;

    default:
        // unknown option
        return;
    }

    sender->setState(index); // report the selected option back to the HA panel
}

void onNumberCommand(HANumeric number, HANumber* sender)
{
    if (!number.isSet()) {
        // the reset command was send by Home Assistant
    } else {
        // you can do whatever you want with the number as follows:
        // uint8_t  numberInt8  = number.toUInt8();
                // you can do whatever you want with the number as follows:
        int8_t numberInt8 = number.toInt8();
        int16_t numberInt16 = number.toInt16();
        int32_t numberInt32 = number.toInt32();
        uint8_t numberUInt8 = number.toUInt8();
        uint16_t numberUInt16 = number.toUInt16();
        uint32_t numberUInt32 = number.toUInt32();
        float numberFloat = number.toFloat();

        delayInterval = numberUInt8*1000;

        Serial.print("New Delay int: ");
        Serial.println(delayInterval);
    }

    sender->setState(number); // report the selected option back to the HA panel
}

void myEnsGas()
{

  if (myENS.checkDataStatus())
  {

    if (printedCompensation == false)
    {
      Serial.println("---------------------------");
      Serial.print("Compensation Temperature: ");
      Serial.println(myENS.getTempCelsius());
      Serial.println("---------------------------");
      Serial.print("Compensation Relative Humidity: ");
      Serial.println(myENS.getRH());
      Serial.println("---------------------------");
      printedCompensation = true;
      delay(500);
    }

    aqi = myENS.getAQI();
    Serial.print("Air Quality Index (1-5) : ");
    Serial.println(aqi);

    Tvoc = myENS.getTVOC();
    Serial.print("Total Volatile Organic Compounds: ");
    Serial.print(Tvoc);
    Serial.println("ppb");

    ECO2 = myENS.getECO2();
    Serial.print("CO2 concentration: ");
    Serial.print(ECO2);
    Serial.println("ppm");

    delay(50);
    oled_display("ESP-AIR-Quality ", 0, 0, 1, true);
    oled_display("Temp: ", 0, 17, 1, false);
    oled_display(String(tempC), 50, 17, 1, false);
    oled_display("Hum: ", 0, 26, 1, false);
    oled_display(String(rh), 50, 26, 1, false);
    delay(50);

    oled_display("Idx: ", 0, 35, 1, false);
    oled_display(String(aqi), 50, 35, 1, false);
    oled_display("TVO: ", 0, 44, 1, false);
    oled_display(String(Tvoc), 50, 44, 1, false);
    oled_display("CO2: ", 0, 53, 1, false);
    oled_display(String(ECO2), 50, 53, 1, false);
    delay(50);

    if (aqi != lastidx)
    {
      lastidx = aqi;
      airidx.setValue(aqi);
    }

    int dif = abs(Tvoc - lasttvoc);

    Serial.print("Dif: ");
    Serial.println(dif);

    if (dif > 20)
    {
      airtvoc.setValue(Tvoc);
      lasttvoc = Tvoc;
    }
    else
    {
      Serial.println("Not dif ");
    }

    dif = abs(ECO2 - lasteco2);

    if (dif > 20)
    {
      aireco2.setValue(ECO2);
      lasteco2 = ECO2;
    }
    else
    {
      Serial.println("Not dif ");
    }
  }

}

void setup()
{
  Serial.begin(115200);

  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500); // waiting for the connection
  }
  Serial.println();
  Serial.println("Connected to the network");

  // initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  if (!myENS.begin())
  {
    Serial.println("Air Quality Sensor did not begin.");
    while (1)
      ;
  }

  while (aht20.begin() != true)
  {
    Serial.println(F("AHT2x not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free

    delay(5000);
  }
  tempC = aht20.readTemperature();
  rh = aht20.readHumidity();
  // Give values to Air Quality Sensor.
  myENS.setTempCompensationCelsius(tempC);
  myENS.setRHCompensationFloat(rh);

  // Set to standard operation
  // Others include SFE_ENS160_DEEP_SLEEP and SFE_ENS160_IDLE
  myENS.setOperatingMode(SFE_ENS160_STANDARD);

  // There are four values here:
  // 0 - Operating ok: Standard Opepration
  // 1 - Warm-up: occurs for 3 minutes after power-on.
  // 2 - Initial Start-up: Occurs for the first hour of operation.
  //												and only once in sensor's lifetime.
  // 3 - No Valid Output
  ensStatus = myENS.getFlags();
  Serial.print("Gas Sensor Status Flag: ");
  Serial.println(ensStatus);

  device.setName("AirQENS");
  device.setSoftwareVersion("1.0.0");

  // refreshTime.onCommand(onNumberCommand);

  // // Optional configuration
  // refreshTime.setIcon("mdi:timer-settings-outline");
  // refreshTime.setName("Refresh Timer");

  number.onCommand(onNumberCommand);

  // Optional configuration
  number.setIcon("mdi:timer-settings-outline");
  number.setName("Delay Int");
  number.setMax(1800);

  airidx.setIcon("mdi:numeric");
  airidx.setName("Air Index");
  airidx.setUnitOfMeasurement("idx");

  airtvoc.setIcon("mdi:molecule-co");
  airtvoc.setName("Air TVOC");
  airtvoc.setUnitOfMeasurement("ppm");

  aireco2.setIcon("mdi:molecule-co2");
  aireco2.setName("Air ECO2");
  aireco2.setUnitOfMeasurement("ppm");


    selectv2.setOptions("Low;Medium;High"); // use semicolons as separator of options
    selectv2.onCommand(onSelectCommand);

    selectv2.setIcon("mdi:home"); // optional
    selectv2.setName("My dropdown"); // optional

  mqtt.begin(BROKER_ADDR, "fst", "asxcvfrbQ1!");
}

void loop()
{

  mqtt.loop();

  
  if (!client.connected())
  {
    oled_display("Connect WiFi ", 20, 35, 1, true);
  }
  else
  {

      unsigned long currentMillis = millis();  // Get the current time

  // Check if the desired delay interval has passed
  if (currentMillis - previousMillis >= delayInterval) {
    // Perform your task here
    myEnsGas();
    // Update the previous time the delay occurred
    previousMillis = currentMillis;
  }


  }

  // client.loop();
  // delay(1000);  // <- fixes some issues with WiFi stability
}