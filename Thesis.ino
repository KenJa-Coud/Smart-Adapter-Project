#include <Wire.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <U8g2lib.h>
#include <PZEM004Tv30.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Firebase configuration
#define FIREBASE_AUTH "AIzaSyC6XcA4M2waxYVsHIuh0l-WYAbzAH5H_ts"
#define FIREBASE_DATABASE_URL "https://smart-adapter-971f8-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define WIFI_SSID "K E B B PC"
#define WIFI_PASSWORD "onlineclass"

// OLED display configuration
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21); // ESP32 I2C pins
char buffer[32];

// PZEM-004T v3.0 configuration
PZEM004Tv30 pzem(&Serial2, 16, 17); // RX, TX pins for ESP32

// Firebase data object
FirebaseData fbData;
FirebaseJson json;
FirebaseConfig config;
FirebaseAuth auth;

float cumulativeEnergy = 0.0; // Variable to store cumulative energy in Wh
float hourlyEnergy = 0.0; // Variable to store hourly energy in Wh
const float hourlyRate = 10; // Hourly rate in ₱
unsigned long lastResetTime = 0; // To keep track of the last reset time
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
String userUID; // Variable to store the user UID

void setup() {
  // Pin for Relay
  pinMode(25, OUTPUT);

  // Initialize serial communication
  Serial.begin(115200);

  // Initialize OLED display
  u8g2.begin();

  // Initialize PZEM-004T v3.0
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX, TX pins

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  // Configure Firebase
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_DATABASE_URL; // Add the database URL

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
    userUID = auth.token.uid.c_str(); // Retrieve the user UID
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Check Firebase connection
  if (!Firebase.ready()) {
    Serial.println("Failed to connect to Firebase");
    return;
  }
}

void loop() {
  float voltage, current, power, energy;

  // Read data from PZEM-004T v3.0
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();

  // Accumulate the energy consumption
  cumulativeEnergy += energy;
  hourlyEnergy += energy;

  // Calculate energy in kWh
  float energy_kWh = cumulativeEnergy / 1000.0;

  // Calculate bill rate based on accumulated energy consumed
  float billRate = energy_kWh * hourlyRate;

  // Calculate hourly bill rate
  float hourlyBillRate = (hourlyEnergy / 1000.0) * hourlyRate;

  // Reset hourly energy every hour
  if (millis() - lastResetTime >= 3600000) {
    hourlyEnergy = 0.0;
    lastResetTime = millis();
  }

  // Print the readings to the Serial Monitor
  Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current); Serial.println(" A");
  Serial.print("Power: "); Serial.print(power); Serial.println(" W");
  Serial.print("Energy: "); Serial.print(cumulativeEnergy); Serial.println(" Wh");
  Serial.print("Energy (kWh): "); Serial.print(energy_kWh); Serial.println(" kWh");
  Serial.print("Bill Rate: ₱"); Serial.print(billRate); Serial.println();
  Serial.print("Hourly Bill Rate: ₱"); Serial.print(hourlyBillRate); Serial.println();

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

  // Send data to Firebase
  json.clear();
  json.add("voltage", voltage);
  json.add("current", current);
  json.add("power", power);
  json.add("energy_kWh", energy_kWh);
  json.add("billRate", billRate);
  
  // Use Firebase.RTDB.setJSON instead of Firebase.setJSON
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    if (Firebase.RTDB.setJSON(&fbData, "users/" + userUID + "/Data/", &json)) {
      Serial.println("Data sent to Firebase successfully");

    } else {
      Serial.println("Failed to send data to Firebase");
      Serial.println(fbData.errorReason());
    }
  }

  delay(60000); // Update every 2 seconds
}