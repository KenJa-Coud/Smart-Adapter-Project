// Template ID, Device Name, and Auth Token provided by the Blynk.Cloud
#define BLYNK_TEMPLATE_ID "TMPL6VPn-lq0r"
#define BLYNK_TEMPLATE_NAME "Thesis"
#define BLYNK_AUTH_TOKEN "1tNMy8Gpqq4-QS89RGWVd8DhR4wtFsu4"

#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <U8g2lib.h>
#include <PZEM004Tv30.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Math Geeks";
char pass[] = "cjslcjsl11!"; 

// OLED display configuration
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21); // ESP32 I2C pins
char buffer[32];

// PZEM-004T v3.0 configuration
PZEM004Tv30 pzem(&Serial2, 16, 17); // RX, TX pins for ESP32

// Relay manual switching from Blynk
bool isManualSwitchOn = false; // Variable to store the state of the manual switch

BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  digitalWrite(25, pinValue);
  isManualSwitchOn = (pinValue == 1); // Update the manual switch state
}



float cumulativeEnergy = 0.0; // Variable to store cumulative energy in Wh
float hourlyEnergy = 0.0; // Variable to store hourly energy in Wh
const float hourlyRate = 10; // Hourly rate in ₱
unsigned long lastResetTime = 0; // To keep track of the last reset time

void setup() {
  // Pin for Relay
  pinMode(25, OUTPUT);
  pinMode(34, HIGH);

  // Initialize serial communication
  Serial.begin(115200);

  // Initialize Blynk connection
  Blynk.begin(auth, ssid, pass);

  // Initialize OLED display
  u8g2.begin();

  // Initialize PZEM-004T v3.0
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX, TX pins
}

void loop() {
  Blynk.run(); // Run Blynk

  float voltage, current, power, energy;

  // If the manual switch is on, set current, voltage, and power to 0
  if (isManualSwitchOn) {
    voltage = 0.0;
    current = 0.0;
    power = 0.0;
    energy = 0.0;
  } else {
    // Read data from PZEM-004T v3.0
    voltage = pzem.voltage();
    current = pzem.current();
    power = pzem.power(); // Instantaneous power in watts
    energy = pzem.energy(); // Energy in watt-hours

    // Accumulate the energy consumption
    cumulativeEnergy += energy; // Accumulate energy in Wh
    hourlyEnergy += energy; // Accumulate hourly energy in Wh
  }

  // Calculate energy in kWh
  float energy_kWh = cumulativeEnergy / 1000.0;

  // Calculate bill rate based on accumulated energy consumed
  float billRate = energy_kWh * hourlyRate; // Replace hourlyRate with the actual cost per kWh

  // Calculate hourly bill rate
  float hourlyBillRate = (hourlyEnergy / 1000.0) * hourlyRate;

  // Reset hourly energy every hour
  if (millis() - lastResetTime >= 3600000) { // 3600000 ms = 1 hour
    hourlyEnergy = 0.0;
    lastResetTime = millis();
  }


  // Display data on OLED using U8g2 library
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(2, 8, "Adapt Tech");
  u8g2.drawLine(0, 9, 127, 9);

  u8g2.print("Energy: "); u8g2.print(energy_kWh); u8g2.println(" kWh");
  u8g2.setCursor(3, 20);
  u8g2.print("Bill Rate: "); u8g2.print(billRate); u8g2.println(" ₱");
  u8g2.setCursor(3, 30);

  u8g2.sendBuffer();

  // Send data to Blynk
  Blynk.virtualWrite(V1, voltage); // Send voltage to virtual pin V1
  Blynk.virtualWrite(V2, current); // Send current to virtual pin V2
  Blynk.virtualWrite(V3, power);   // Send power to virtual pin V3
  Blynk.virtualWrite(V4, energy_kWh);  // Send cumulative energy in kWh to virtual pin V4
  Blynk.virtualWrite(V5, billRate); // Send bill rate to virtual pin V5

  delay(60000); // Update every 60 seconds
}