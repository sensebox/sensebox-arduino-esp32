#define DISPLAY_ENABLED

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#include <Update.h>
#include <Wire.h>
#ifdef DISPLAY_ENABLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <Adafruit_NeoPixel.h>
 Adafruit_NeoPixel rgb_led_1= Adafruit_NeoPixel(1, 1,NEO_GRB + NEO_KHZ800);


#endif
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

String ssid;
uint8_t mac[6];


// Create an instance of the server
WebServer server(80);
bool displayEnabled;

const int BUTTON_PIN = 0;   // GPIO für den Button
volatile unsigned long lastPressTime = 0;  // Zeitpunkt des letzten Drucks
volatile bool doublePressDetected = false; // Flag für Doppeldruck
const unsigned long doublePressInterval = 500; // Max. Zeit (in ms) zwischen zwei Drücken für Doppeldruck
volatile int pressCount = 0;  // Zählt die Button-Drucke

void IRAM_ATTR handleButtonPress() {
    unsigned long currentTime = millis();  // Hole aktuelle Zeit

    // Vermeidung von Prellen: Wenn der aktuelle Druck zu nahe am letzten liegt, ignoriere ihn
    if (currentTime - lastPressTime > 50) {
        pressCount++;  // Zähle den Button-Druck
        lastPressTime = currentTime;  // Aktualisiere die Zeit des letzten Drückens

        // Überprüfe, ob dies der zweite Druck innerhalb des Double-Press-Intervalls ist
        if (pressCount == 2 && (currentTime - lastPressTime <= doublePressInterval)) {
            doublePressDetected = true;  // Doppeldruck erkannt
            pressCount = 0;  // Zähler zurücksetzen
        }
    }
}

// Funktion zum Wechseln der Boot-Partition auf OTA1
void setBootPartitionToOTA0() {
    const esp_partition_t* ota0_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

    if (ota0_partition) {
        // Setze OTA1 als neue Boot-Partition
        esp_ota_set_boot_partition(ota0_partition);
        Serial.println("Boot partition changed to OTA0. Restarting...");

        // Neustart, um von der neuen Partition zu booten
        esp_restart();
    } else {
        Serial.println("OTA1 partition not found!");
    }
}

void setupDisplay() {
  displayEnabled =  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  if(displayEnabled) {
    display.display();
    delay(100);
    display.clearDisplay();
  }
}

void displaySSID() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.println("Verbinde dich mit dem Access Point:");
  display.setTextSize(1);
  display.println();
  display.setCursor(10, 32);
  display.println(ssid);
  display.display();
  display.clearDisplay();
}

void setupWiFi() {
  WiFi.macAddress(mac);
  char macLastFour[5];
  snprintf(macLastFour, sizeof(macLastFour), "%02X%02X", mac[4], mac[5]);
  ssid = "senseBox:" + String(macLastFour);

  // Definiere die IP-Adresse, Gateway und Subnetzmaske
  IPAddress local_IP(192,168,1,1);      // Die neue IP-Adresse
  IPAddress gateway(192,168,1,1);       // Gateway-Adresse (kann gleich der IP des APs sein)
  IPAddress subnet(255,255,255,0);      // Subnetzmaske

  // Setze die IP-Adresse, Gateway und Subnetzmaske des Access Points
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Starte den Access Point
  WiFi.softAP(ssid.c_str());
}

void setupOTA() {
  // Handle updating process
  server.on("/sketch", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
}

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  rgb_led_1.begin();
  rgb_led_1.setBrightness(30);
  rgb_led_1.setPixelColor(0,rgb_led_1.Color(51, 51, 255));
  rgb_led_1.show();

  // Button-Pin als Input konfigurieren
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Interrupt für den Button
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
  
  #ifdef DISPLAY_ENABLED
  setupDisplay();
  #endif
  setupWiFi();
  // Set the ESP32 as an access point
  setupOTA();
  server.begin();
}

void loop() {
  // Handle client requests
  server.handleClient();
  
  #ifdef DISPLAY_ENABLED
    displaySSID();
  #endif

    if (doublePressDetected) {
        Serial.println("Doppeldruck erkannt!");
        setBootPartitionToOTA0();
        #ifdef DISPLAY_ENABLED
        display.clearDisplay();
        display.display();
        delay(50);
        #endif
        // Neustart, um von der neuen Partition zu booten
        esp_restart();
      
    }
}
  