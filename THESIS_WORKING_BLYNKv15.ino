// Template ID, Device Name, and Auth Token provided by the Blynk.Cloud
#define BLYNK_TEMPLATE_ID "TMPL6w3Zn2I7S"
#define BLYNK_TEMPLATE_NAME "Smart Adapter"
#define BLYNK_AUTH_TOKEN "CunJjGxmCNMlBzEauHEhRgNm0bircJQY"

#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <U8g2lib.h>
#include <PZEM004Tv30.h>
#include <EEPROM.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "PLDTHOMEFIBRJyKcd";
char pass[] = "PLDTWIFIRfsiE"; 

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

float totalAccumulatedEnergy = 0.0; // Variable to store total accumulated energy in Wh
float hourlyEnergy = 0.0; // Variable to store hourly energy in Wh
float billRate = 0.0; // Variable to store bill rate
const float hourlyRate = 10; // Hourly rate in ₱
unsigned long lastResetTime = 0; // To keep track of the last reset time
unsigned long previousMillis = 0; // Store the last time the power consumption was updated
const long interval = 121000; // Update interval in milliseconds (121 seconds)

// EEPROM addresses for storing accumulated energy and bill rate
const int EEPROM_SIZE = 512;
const int ENERGY_ADDR = 0;
const int BILL_RATE_ADDR = 4;

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

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Read stored values from EEPROM
  EEPROM.get(ENERGY_ADDR, totalAccumulatedEnergy);
  EEPROM.get(BILL_RATE_ADDR, billRate);
}

void loop() {
  Blynk.run(); // Run Blynk

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

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

      // Calculate the energy consumed during the update interval
      float intervalEnergy = power * (interval / 3600000.0); // Convert milliseconds to hours

      // Accumulate the energy consumption
      totalAccumulatedEnergy += intervalEnergy; // Accumulate energy in Wh
      hourlyEnergy += intervalEnergy; // Accumulate hourly energy in Wh

      // Calculate energy in kWh
      float energy_kWh = totalAccumulatedEnergy / 1000.0;

      // Calculate bill rate based on accumulated energy consumed
      billRate = energy_kWh * hourlyRate; // Replace hourlyRate with the actual cost per kWh

      // Store updated values in EEPROM
      EEPROM.put(ENERGY_ADDR, totalAccumulatedEnergy);
      EEPROM.put(BILL_RATE_ADDR, billRate);
      EEPROM.commit();
    }

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

    u8g2.print("Energy: "); u8g2.print(totalAccumulatedEnergy / 1000.0); u8g2.println(" kWh");
    u8g2.setCursor(3, 20);
    u8g2.print("Bill Rate: "); u8g2.print(billRate); u8g2.println(" ₱");
    u8g2.setCursor(3, 30);

    u8g2.sendBuffer();

    // Send data to Blynk
    Blynk.virtualWrite(V1, voltage); // Send voltage to virtual pin V1
    Blynk.virtualWrite(V2, current); // Send current to virtual pin V2
    Blynk.virtualWrite(V3, power);   // Send power to virtual pin V3
    Blynk.virtualWrite(V4, totalAccumulatedEnergy / 1000.0);  // Send cumulative energy in kWh to virtual pin V4
    Blynk.virtualWrite(V5, billRate); // Send bill rate to virtual pin V5
    Blynk.virtualWrite(V6, hourlyEnergy / 1000.0); // Send hourly energy in kWh to virtual pin V6
  }
}