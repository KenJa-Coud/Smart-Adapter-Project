#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <U8g2lib.h>

// Replace with your network credentials for the AP
const char* ap_ssid = "ESP32_Config";
const char* ap_password = "12345678";

// Initialize the OLED display (adjust pins if necessary)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21); // ESP32 I2C pins

WebServer server(80);

// HTML form to capture WiFi credentials
const char* htmlForm = R"(
<html>
  <body>
    <h2>Configure WiFi</h2>
    <form action="/save" method="POST">
      SSID: <input type="text" name="ssid"><br>
      Password: <input type="password" name="password"><br>
      <input type="submit" value="Save">
    </form>
  </body>
</html>
)";

void displayCenteredText(const char* line1, const char* line2 = "") {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth(line1)) / 2, 25, line1);
  if (strlen(line2) > 0) {
    u8g2.drawStr((128 - u8g2.getStrWidth(line2)) / 2, 45, line2);
  }
  u8g2.sendBuffer();
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize OLED display
  u8g2.begin();

  // Display welcome message on OLED
  displayCenteredText("Welcome to", "AdapTech");
  delay(3000);

  // Initialize EEPROM with size 512 bytes
  EEPROM.begin(512);

  // Check if WiFi credentials are stored in EEPROM
  String ssid = readStringFromEEPROM(0);
  String password = readStringFromEEPROM(100);

  if (ssid.length() > 0 && password.length() > 0) {
    // Try to connect to WiFi
    displayCenteredText("Connecting to", "WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    displayCenteredText("Wi-Fi Connection", "Successful");
    delay(3000);
  } else {
    // Set up the ESP32 as an Access Point
    Serial.println("Setting up Access Point...");
    bool ap_success = WiFi.softAP(ap_ssid, ap_password);

    if (ap_success) {
      Serial.println("Access Point created successfully!");
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
      
      // Display instruction messages on OLED
      displayCenteredText("Please Connect the", "SmaDap Device");
      delay(4000);
      displayCenteredText("Open your PC or", "Phone and connect");
      delay(4000);
      displayCenteredText("Connect to", WiFi.softAPIP().toString().c_str());

      // Start the web server
      server.on("/", handleRoot);
      server.on("/save", handleSave);
      server.begin();
      Serial.println("Web server started");
    } else {
      Serial.println("Failed to create Access Point.");
    }
  }
}

void loop() {
  // Handle incoming clients
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", htmlForm);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  // Save the credentials to EEPROM
  writeStringToEEPROM(0, ssid);
  writeStringToEEPROM(100, password);
  EEPROM.commit();

  // Respond to the client
  server.send(200, "text/html", "<html><body><h2>Credentials Saved</h2><p>ESP32 will restart and connect to the WiFi network.</p></body></html>");

  // Restart the ESP32 to apply the new settings
  delay(2000);
  ESP.restart();
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int len = EEPROM.read(addrOffset);
  char data[len + 1];
  for (int i = 0; len > 0 && i < len; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[len] = '\0';
  return String(data);
}
