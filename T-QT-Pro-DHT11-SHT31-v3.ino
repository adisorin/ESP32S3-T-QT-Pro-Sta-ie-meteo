/*MIT License

Copyright (c) 2026 adisorin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
/* https://github.com/adisorin?tab=repositories */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Arduino_GFX_Library.h>
#include <time.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_SHT31.h> // Bibliotecă adăugată pentru SHT31

//////////////////////////////
// CULORI RGB565
#define GC9107_BLACK   0x0000
#define GC9107_WHITE   0xFFFF
#define GC9107_RED     0xF800
#define GC9107_GREEN   0x07E0
#define GC9107_BLUE    0x001F
#define GC9107_YELLOW  0xFFE0
#define GC9107_DARKGREY  0x4208
#define GC9107_CYAN    0x07FF

//////////////////////////////
// TFT PINS
#define TFT_MOSI 2
#define TFT_SCLK 3
#define TFT_CS   5
#define TFT_DC   6
#define TFT_RST  1
#define TFT_BL   4

//////////////////////////////
// TFT INIT
Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9107(bus, TFT_RST, TFT_BL, true, 128, 128);

//////////////////////////////
// WIFI
WiFiMulti wifiMulti;

//////////////////////////////
// SENZORI
#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// SHT31 PINS (I2C)
#define I2C_SDA 17  // Conectează pinul SDA al SHT31 aici
#define I2C_SCL 18  // Conectează pinul SCL al SHT31 aici
Adafruit_SHT31 sht31 = Adafruit_SHT31();

bool isSHT31 = false; // Flag pentru detectarea automată

//////////////////////////////
// NTP
const char* ntpServer = "pool.ntp.org";

//////////////////////////////
// TIMERE
unsigned long lastWifiCheck = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastDHTUpdate = 0;

bool timeConfigured = false;

////////////////////////////////////////////////////////////
//////////////////// INDICATOR WIFI ////////////////////////

void afiseazaNivelWiFi() {
  int latime = 4;
  int spatiu = 2;
  int inaltimeMax = 15;
  int totalWidth = 5 * (latime + spatiu);
  int x = 128 - totalWidth - 4;
  int y = inaltimeMax + 4;

  int rssi = WiFi.RSSI();
  int nivel = 0;

  if (WiFi.status() != WL_CONNECTED) nivel = 0;
  else if (rssi > -50) nivel = 5;
  else if (rssi > -60) nivel = 4;
  else if (rssi > -70) nivel = 3;
  else if (rssi > -80) nivel = 2;
  else if (rssi > -90) nivel = 1;

  gfx->fillRect(x - 2, 0, totalWidth + 4, inaltimeMax + 6, GC9107_BLACK);

  for (int i = 0; i < 5; i++) {
    int h = (i + 1) * (inaltimeMax / 5);
    uint16_t culoare = (i < nivel) ? GC9107_GREEN : GC9107_DARKGREY;
    gfx->fillRect(x + i * (latime + spatiu), y - h, latime, h, culoare);
  }
}

void drawStaticUI() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 5);
  gfx->println("Temp & Humid");
}

//////////////////////////////

void setup() {

  Serial.begin(115200);

  // Initializare Backlight TFT
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initializare SPI si Ecran
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  gfx->begin();
  gfx->fillScreen(GC9107_BLACK);

  // Desenare interfata statica
  drawStaticUI();

  // --- DETECTIE AUTOMATA SENZOR ---
  
  // Initializare bus I2C pentru SHT31
  Wire.begin(I2C_SDA, I2C_SCL); 

  // Incearca sa porneasca SHT31 (adresa default 0x44)
  if (sht31.begin(0x44)) { 
    isSHT31 = true;
    Serial.println("SHT31 detectat pe I2C!");
  } else {
    // Daca SHT31 nu raspunde, configuram DHT11
    isSHT31 = false;
    dht.begin(); 
    Serial.println("SHT31 nu a fost gasit, se foloseste DHT11 pe pin 16");
  }

  // ==== Wifi network ssid and password ====
  wifiMulti.addAP("*********"); // Changeme
  wifiMulti.addAP("*********"); // Changeme

  // Mesaj conectare pe ecran
  gfx->setCursor(10, 35);
  gfx->setTextColor(GC9107_YELLOW);
  gfx->setTextSize(1);
  gfx->println("Connecting WiFi...");
}


void checkWiFi() {
  wifiMulti.run();
  if (WiFi.status() == WL_CONNECTED) {
    gfx->fillRect(0, 35, 128, 15, GC9107_BLACK);
    gfx->setCursor(20, 35);
    gfx->setTextColor(GC9107_GREEN);
    gfx->setTextSize(1);
    gfx->println("WiFi Connected");

    if (!timeConfigured) {
      configTime(0, 0, ntpServer);
      setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
      tzset();
      timeConfigured = true;
    }
  } else {
    for (int i = 0; i < 10; i++) {
      gfx->fillRect(0, 35, 128, 15, GC9107_BLACK);
      gfx->setCursor(15, 35);
      gfx->setTextColor(GC9107_RED);
      gfx->setTextSize(1);
      gfx->println("...WRONG WIFI...");
      delay(500);
      gfx->fillRect(0, 35, 128, 15, GC9107_BLACK);
      delay(500);
    }
  }
}

void afiseazaCeas() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  gfx->fillRect(0, 100, 128, 25, GC9107_BLACK);
  gfx->setTextColor(GC9107_YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor(15, 105);
  gfx->print(buffer);
}

bool blinkState = false;

void afiseazaDHT() {
  float temp, hum;

  // Încercăm să citim de la senzorul activat la boot
  if (isSHT31) {
    temp = sht31.readTemperature();
    hum = sht31.readHumidity();
  } else {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
  }

  // Schimbăm starea pentru clipire/alternare la fiecare apel (2 secunde)
  blinkState = !blinkState;

  gfx->fillRect(0, 50, 128, 55, GC9107_BLACK);

  // VERIFICARE EROARE (Dacă senzorul curent nu răspunde)
  if (isnan(temp) || isnan(hum)) {
    gfx->setTextSize(2);
    
    

    // ALTERNARE MESAJE: Dacă nu avem date, afișăm pe rând eroarea de SHT și DHT
    if (blinkState) {
      gfx->setCursor(20, 55);
      gfx->setTextColor(GC9107_RED);
      gfx->println("SHT ERR");
    } else {gfx->setCursor(20, 75);
      gfx->setTextColor(GC9107_BLUE);
      gfx->println("DHT ERR");
    }
    return; // Oprim execuția aici dacă avem eroare
  }

  // --- AFIȘARE NORMALĂ (Dacă senzorul funcționează) ---
  gfx->setTextSize(2);
  gfx->setCursor(27, 55);
  gfx->setTextColor(GC9107_GREEN);
  gfx->print(temp, 1);
  gfx->print(" C");
  
  if ((temp < 18.0 || temp > 26.0) && blinkState) {
    gfx->setCursor(100, 55);
    gfx->setTextColor(GC9107_RED);
    gfx->print(" !"); 
  }

  gfx->setCursor(37, 80);
  gfx->setTextColor(GC9107_CYAN);
  gfx->print(hum, 0);
  gfx->print(" %");

  if ((hum < 30.0 || hum > 60.0) && blinkState) {
    gfx->setCursor(100, 80);
    gfx->setTextColor(GC9107_RED);
    gfx->print("!");
  }
}


void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastWifiCheck > 10000) {
    checkWiFi();
    lastWifiCheck = currentMillis;
  }

  if (currentMillis - lastClockUpdate > 1000) {
    afiseazaCeas();
    afiseazaNivelWiFi();
    lastClockUpdate = currentMillis;
  }

  if (currentMillis - lastDHTUpdate > 2000) {
    afiseazaDHT();
    lastDHTUpdate = currentMillis;
  }
}
