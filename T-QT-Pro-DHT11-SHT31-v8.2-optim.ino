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
SOFTWARE.
*/

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
#include <Adafruit_SHT31.h>
#include <Adafruit_BME680.h>
#include <WebServer.h>
#include <DNSServer.h>

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
// BUTOANE
#define BTN_IO0   0
#define BTN_IO47  47

//////////////////////////////
// TFT INIT
Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9107(bus, TFT_RST, TFT_BL, true, 128, 128);

//////////////////////////////
// WIFI & SERVER
WiFiMulti wifiMulti;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

//////////////////////////////
// SENZORI
#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define I2C_SDA 17
#define I2C_SCL 18
Adafruit_SHT31 sht31 = Adafruit_SHT31();
bool isSHT31 = false;
Adafruit_BME680 bme;
bool isBME680 = false;

//////////////////////////////
// SSID DINAMIC
String ssidCurent = "ESP32S3-T/H-senzor";
unsigned long lastSSIDUpdate = 0;
const char* apPassword = "12345678"; // minim 8 caractere

//////////////////////////////
// DATE SENZOR (pentru web)
float lastTemp = NAN;
float lastHum = NAN;
float lastPressure = NAN;
float lastGas = NAN;

//////////////////////////////
// TIMERE
unsigned long lastDHTUpdate = 0;
unsigned long lastBME680Update = 0;

//////////////////////////////
// PAGINA ACTUALA
enum Pagina { MAIN_UI, SYSTEM_INFO };
Pagina paginaCurenta = MAIN_UI;

//////////////////////////////
// BLINK ALERT
bool blinkState = false;
unsigned long lastBlinkTime = 0;
long blinkInterval = 500; // ms pentru blink

////////////////////////////////////////////////////////////
// FUNCTII TFT
void drawStaticUI() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 1);
  gfx->println("ESP CLIMA");
}


float corectiePresiune(float presiune, float altitudine) {
  return presiune * pow(1.0 - (altitudine / 44330.0), -5.255);
}
void afiseazaDHT() {
  float temp, hum;
  if(isBME680){
    if (!bme.performReading()) { 
      temp = NAN; 
      hum = NAN; 
    }
    else {
      temp = bme.temperature;
      hum  = bme.humidity;

      float presAbs = bme.pressure / 100.0;   // presiune absolută (hPa)
      lastPressure = corectiePresiune(presAbs, 132.0) + 0.3; // corecție nivel mare + etajul 3 Lugoj

      lastGas = bme.gas_resistance / 1000.0;
    }
  } 
  else if(isSHT31){
    temp = sht31.readTemperature();
    hum  = sht31.readHumidity();

    // fără senzor de presiune
    lastPressure = NAN;
    lastGas = NAN;
  } 
  else {
    temp = dht.readTemperature();
    hum  = dht.readHumidity();

    // fără senzor de presiune
    lastPressure = NAN;
    lastGas = NAN;
  }
  static bool lastError = false;
  bool currentError = isnan(temp) || isnan(hum);
  temp = ((temp - 1.6) * 1.003) + 0;// Calibrare T
  hum = ((hum + 5.0) * 1.02);// Calibrare H
  if(hum > 100) hum = 100;
  if(hum < 0)   hum = 0;

  // 🔥 dacă se schimbă starea, curăță o singură dată
  if(currentError != lastError){
    gfx->fillRect(0, 30, 128, 128, GC9107_BLACK);
    lastError = currentError;
  }
  // =========================
  // 🔴 EROARE (fără flicker)
  // =========================
  if(currentError){
    gfx->setTextSize(2);
    // șterge doar linia unde NU scriem
    if(blinkState){
      gfx->setCursor(20, 75);
      gfx->setTextColor(GC9107_BLACK, GC9107_BLACK);
      gfx->print("       "); // clear linie
      gfx->setCursor(20, 55);
      gfx->setTextColor(GC9107_RED, GC9107_BLACK);
      gfx->print("SHT ERR");
    } else {
      gfx->setCursor(20, 55);
      gfx->setTextColor(GC9107_BLACK, GC9107_BLACK);
      gfx->print("       ");
      gfx->setCursor(20, 75);
      gfx->setTextColor(GC9107_BLUE, GC9107_BLACK);
      gfx->print("DHT ERR");
    } 
    if(blinkState){
      gfx->setCursor(20, 95);
      gfx->setTextColor(GC9107_BLACK, GC9107_BLACK);
      gfx->print("        "); // clear
      gfx->setCursor(20, 95);
      gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
      gfx->print("BME ERR");
      }
    return;
  }
  lastTemp = temp;
  lastHum  = hum;
  // =========================
  // 🌡️ TEMPERATURA (overwrite)
  // =========================
  gfx->setTextSize(2);
  gfx->setCursor(27, 35);
  gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
  gfx->print("      "); // clear
  gfx->setCursor(27, 35);
  gfx->print(temp, 1);
  gfx->drawCircle(27 + 55, 32 + 3, 2, GC9107_GREEN);
  gfx->setCursor(27 + 60, 35);
  gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
  gfx->print("C ");
  // =========================
  // 💧 HUMIDITY
  // =========================
  gfx->setCursor(27, 60);
  gfx->setTextColor(GC9107_CYAN, GC9107_BLACK);
  gfx->print("      ");
  gfx->setCursor(27, 60);
  gfx->print(hum,1); 
  gfx->print(" %");
  // =========================
  // 🌍 BME680
  // =========================
  if(isBME680){
    gfx->setCursor(5, 85);
    gfx->setTextColor(GC9107_YELLOW, GC9107_BLACK);
    gfx->print("        ");
    gfx->setCursor(5, 85);
    gfx->print(lastPressure,0);
    gfx->print(" hPa");
    gfx->setCursor(100, 85);

    if (lastPressure > 1020) {
        gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
        gfx->print(" H");
    } 
    else if (lastPressure <= 1020 && lastPressure > 1015) {
        gfx->setTextColor(GC9107_CYAN, GC9107_BLACK);
        gfx->print(" M");
    } 
    else { // sub 1015
        gfx->setTextColor(GC9107_BLUE, GC9107_BLACK); // am pus roșu pentru contrast, sau ce culoare preferi
        gfx->print(" L");
    }
    gfx->setCursor(5, 110);
    gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
    gfx->print(lastGas, 0);
    gfx->print(" kOhm");

    // Logica identică cu cea de la presiune pentru indicatorul de gaz
    if (lastGas <= 450 && lastGas >= 150) {
        gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
        gfx->print(" G ");
    } 
    else if (lastGas <= 150 && lastGas >= 50) { 
        // Acoperă tot intervalul de mijloc (inclusiv zona 50-80 care lipsea)
        gfx->setTextColor(GC9107_YELLOW, GC9107_BLACK);
        gfx->print(" K ");
    } 
    else if (lastGas <= 50 && lastGas >= 10) { 
        // Acoperă tot intervalul de mijloc (inclusiv zona 50-80 care lipsea)
        gfx->setTextColor(GC9107_CYAN, GC9107_BLACK);
        gfx->print(" M ");
    } 
    else { // Tot ce este sub 50
        gfx->setTextColor(GC9107_RED, GC9107_BLACK);
        gfx->print(" D ");
    }
  }
  // =========================
  // ❗ BLINK ALERT FĂRĂ FLICKER
  // =========================
  if(temp<18.0 || temp>26.0){
    gfx->setCursor(100,35);
    if(blinkState){
      gfx->setTextColor(GC9107_RED, GC9107_BLACK);
      gfx->print(" !");
    } else {
      gfx->setTextColor(GC9107_BLACK, GC9107_BLACK);
      gfx->print(" !");
    }
  }
  if(hum<30.0 || hum>60.0){
    gfx->setCursor(100,60);
    if(blinkState){
      gfx->setTextColor(GC9107_RED, GC9107_BLACK);
      gfx->print(" !");
    } else {
      gfx->setTextColor(GC9107_BLACK, GC9107_BLACK);
      gfx->print(" !");
    }
  }
  if(millis()-lastSSIDUpdate>5000){ updateSSID(temp,hum); lastSSIDUpdate=millis(); }
}
void checkSensors(){
  // 🔹 verificare BME680
  if(!isBME680){
    if(bme.begin(0x76)){
      isBME680 = true;
    }
  } else {
    // dacă era activ → verifică dacă mai răspunde
    if(!bme.performReading()){
      isBME680 = false;
    }
  }
  // 🔹 verificare SHT31
  if(!isSHT31){
    if(sht31.begin(0x44)){
      isSHT31 = true;
    }
  } else {
    float t = sht31.readTemperature();
    if(isnan(t)){
      isSHT31 = false;
    }
  }
}
////////////////////////////////////////////////////////////
// SSID DINAMIC
void updateSSID(float temp, float hum) {
  char ssidNou[32];
  const char* tStat = (temp < 18) ? "COLD" : (temp > 26) ? "HOT" : "OK";
  const char* hStat = (hum < 30) ? "DRY" : (hum > 60) ? "WET" : "OK";
  snprintf(ssidNou, sizeof(ssidNou), "RLS 3.5:  %.0f *C %s   %.0f%% %s",
  temp, tStat, hum, hStat);

  // NU schimba SSID dacă există clienți conectați
  if (WiFi.softAPgetStationNum() > 0) return;

  if (ssidCurent != String(ssidNou)) {
    ssidCurent = String(ssidNou);
    WiFi.softAPdisconnect(true);
    // 🔥 REAPLICĂ IP-ul !!!
    IPAddress local_ip(192,168,110,1);
    IPAddress gateway(192,168,110,1);
    IPAddress subnet(255,255,255,0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ssidCurent.c_str(), apPassword);
  }
}
////////////////////////////////////////////////////////////
// FUNCTII WEB
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta charset="UTF-8">
<style>
body { font-family: Arial; text-align:center; background:#111; color:white; margin:0; padding:0; }
.card { background:#222; margin:20px; padding:20px; border-radius:15px; box-shadow:0 0 10px #00ffcc; }
.value { font-size:40px; transition:0.5s; }
canvas { background:#000; display:block; margin:20px auto; border-radius:15px; }
.row {
  display: flex;
  gap: 10px;
  margin: 20px;
}
.half {
  flex: 1;
  margin: 0;
  padding: 15px;
}
.half .value {
  font-size: 28px;
}
</style>
</head>
<body>
<h2>💧🌡️ RLS 3.5 🌡️💧</h2>
<div class="card">
  <h3>🌡️Temperature</h3>
  <div id="temp" class="value">No data senzor</div>
</div>
<div class="card">
  <h3>💧Humidity</h3>
  <div id="hum" class="value">No data senzor</div>
</div>
<div class="row">
  <div class="card half">
    <div id="pres" class="value">No data senzor • hPa</div>
  </div>
  <div class="card half">
    <div id="air" class="value">No data senzor • air</div>
  </div>
</div>
<canvas id="chart" width="335" height="150"></canvas>
<script>
let t = [], h = [];
const MAX_POINTS = 500;
const VISIBLE_POINTS = 20;
let scrollOffset = 0;
let isDragging = false;
let dragStartX = 0;
let initialOffset = 0;
const canvas = document.getElementById("chart");
const ctx = canvas.getContext("2d");
// Event listeners pentru scroll / drag
canvas.addEventListener("mousedown", e => { isDragging = true; dragStartX = e.clientX; initialOffset = scrollOffset; });
canvas.addEventListener("mousemove", e => { if(isDragging){ let dx = e.clientX - dragStartX; scrollOffset = initialOffset - Math.round(dx / (canvas.width / VISIBLE_POINTS)); scrollOffset = Math.max(0, Math.min(Math.max(0,t.length - VISIBLE_POINTS), scrollOffset)); draw(); }});
canvas.addEventListener("mouseup", e => { isDragging = false; });
canvas.addEventListener("mouseleave", e => { isDragging = false; });
canvas.addEventListener("touchstart", e => { isDragging = true; dragStartX = e.touches[0].clientX; initialOffset = scrollOffset; });
canvas.addEventListener("touchmove", e => { if(isDragging){ let dx = e.touches[0].clientX - dragStartX; scrollOffset = initialOffset - Math.round(dx / (canvas.width / VISIBLE_POINTS)); scrollOffset = Math.max(0, Math.min(Math.max(0,t.length - VISIBLE_POINTS), scrollOffset)); draw(); }});
canvas.addEventListener("touchend", e => { isDragging = false; });
canvas.addEventListener("touchcancel", e => { isDragging = false; });
// Functie actualizare date periodic
function upd() {
  fetch('/data').then(r => r.json()).then(d => {
  if(d.temp === null){
    temp.innerHTML = "No data senzor";
  } else {
    temp.innerHTML = "🌡️ " + d.temp.toFixed(1) + " °C " + d.tstat;
  }
  if(d.hum === null){
    hum.innerHTML = "No data senzor";
  } else {
    hum.innerHTML = "💧 " + d.hum.toFixed(1) + " % " + d.hstat;
  }
    if(d.tstat==="HOT") temp.style.color="red";
    else if(d.tstat==="COLD") temp.style.color="cyan";
    else temp.style.color="white";

    if(d.hstat==="WET") hum.style.color="#00ccff";
    else if(d.hstat==="DRY") hum.style.color="orange";
    else hum.style.color="white";

    if(d.temp !== null) t.push(d.temp);
    if(d.hum !== null) h.push(d.hum);
    if(t.length > MAX_POINTS){
      t.splice(0,1);
      h.splice(0,1);
    }

    // 🔹 autoscroll doar dacă nu tragi
    if(!isDragging){
      scrollOffset = Math.max(0, t.length - VISIBLE_POINTS);
    }
    // 🔹 interpretare vreme simplă din presiune
    let sky = "Necunoscut";
    let rainChance = 0;
    if (d.pres === null || d.pres < 1) { 
      // Verificăm prima dată dacă senzorul trimite 0
      sky = "No data senzor";
      rainChance = 0;
    } else if (d.pres >= 1020) {
      sky = "☀️ Clear";
      rainChance = 5;
    } else if (d.pres > 1013) {
      sky = "⛅ Partly cloudy";
      rainChance = 20;
    } else if (d.pres > 1005) {
      sky = "☁️ Cloudy";
      rainChance = 45;
    } else if (d.pres > 995) {
      sky = "🌧️ Rain / unstable";
      rainChance = 75;
    } else {
      sky = "⛈️ Strom";
      rainChance = 95;
    }
    // 🔹 CARD STÂNGA (presiune + vreme)
    if(d.pres === null){
      pres.innerHTML = "No data senzor";
    } else {
      pres.innerHTML =
        "🌍 " + d.pres + " hPa<br>" +
        sky + "<br>" +
        "🌧️ " + rainChance + "%";
    }
      // 🔹 interpretare calitate aer
    let airStatus = "Necunoscut";
    let airEmoji = "⚪";
    if (d.gas < 1) { 
      // Punem verificarea pentru eroare/lipsă date prima
      airStatus = "No data senzor";
      airEmoji = "❌"; // Recomand un emoji diferit de "Excelent" pentru eroare
    } else if (d.gas <= 5) {
      airStatus = "Critical / Dangerous";
      airEmoji = "🚨";
    } else if (d.gas <= 9) {
      airStatus = "VERY DANGEROS";
      airEmoji = "☠️";
    } else if (d.gas <= 10) {
      airStatus = "DANGEROS";
      airEmoji = "😷";
    } else if (d.gas <= 50) {
      airStatus = "Moderate";
      airEmoji = "⚠️";
    } else if (d.gas <= 100) {
      airStatus = "GOOD";
      airEmoji = "✅";
    } else if (d.gas <= 450) {
      airStatus = "Excelent";
      airEmoji = "🌿";
    }
    
    // 🔹 CARD DREAPTA (calitate aer)
    if(d.gas === null) {
      air.innerHTML = "No data senzor";
    } else {
      air.innerHTML = 
        airEmoji + " " + d.gas + "%" + " AQI<br>" +
        "Status: " + airStatus + "<br>" +
        "Air";
    }

    // culoare presiune
    if(d.pres >= 1020) pres.style.color = "#00ccff";
    else if(d.pres > 1013) air.style.color = "#00ff99";
    else pres.style.color = "orange";

    // culoare aer
    if(airStatus === "GOOD") air.style.color = "yellow";
    else if(airStatus === "VERY DANGEROS") air.style.color = "magenta";
    else if(airStatus === "DANGEROS") air.style.color = "magenta";
    else if(airStatus === "Moderate") air.style.color = "orange";
    else if(airStatus === "Critical / Dangerous") air.style.color = "red";
    else if(airStatus === "Excelent") air.style.color = "lime";
    draw();
  });
}
setInterval(upd, 10000);
// Functie desenare grafic
function draw() {
  ctx.clearRect(0,0,canvas.width,canvas.height);
  const tempMax = 50, humMax = 100;
  // GRID + AXE
  ctx.strokeStyle="#333"; ctx.lineWidth=1;
  for(let i=0;i<=5;i++){ let y=i*(canvas.height/5); ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(canvas.width,y); ctx.stroke(); }
  ctx.fillStyle="#888"; ctx.font="10px Arial";
  for(let i=0;i<=50;i+=10){ let y=canvas.height-(i/tempMax)*canvas.height; ctx.fillText(i+"°C",10,y); }
  for(let i=0;i<=100;i+=20){ let y=canvas.height-(i/humMax)*canvas.height; ctx.fillText(i+"%", canvas.width-30,y); }
  // Temperatura
  ctx.beginPath();
  for(let i=scrollOffset;i<Math.min(scrollOffset+VISIBLE_POINTS,t.length);i++){
    let x=(i-scrollOffset)*(canvas.width/VISIBLE_POINTS);
    let y=canvas.height - (t[i]/tempMax)*canvas.height;
    if(i==scrollOffset) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.strokeStyle="orange"; ctx.lineWidth=2; ctx.stroke();
  // Umiditate
  ctx.beginPath();
  for(let i=scrollOffset;i<Math.min(scrollOffset+VISIBLE_POINTS,h.length);i++){
    let x=(i-scrollOffset)*(canvas.width/VISIBLE_POINTS);
    let y=canvas.height - (h[i]/humMax)*canvas.height;
    if(i==scrollOffset) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.strokeStyle="#00ccff"; ctx.lineWidth=2; ctx.stroke();
  // Legend
  ctx.fillStyle="orange"; ctx.fillRect(10,5,10,10); ctx.fillStyle="white"; ctx.fillText("Temp",25,14);
  ctx.fillStyle="#00ccff"; ctx.fillRect(314,5,10,10); ctx.fillStyle="white"; ctx.fillText("Hum",286,14);
}
</script>
</body>
</html>
)rawliteral";
  server.send(200,"text/html",html);
}

void handleData() {
  const char* tStat = (lastTemp<18)?"COLD":(lastTemp>26)?"HOT":"OK";
  const char* hStat = (lastHum<30)?"DRY":(lastHum>60)?"WET":"OK";
  const char* airStat = "N/A";
  if(!isnan(lastGas) && lastGas > 0){
    if(lastGas <= 8) airStat = "VERY DANGEROS";
    else if(lastGas <= 10) airStat = "DANGEROS";
    else if(lastGas <= 50) airStat = "POOR";
    else if(lastGas <= 100) airStat = "MODERATE";
    else if(lastGas <= 150) airStat = "GOOD";
    else if(lastGas <= 160) airStat = "VERY GOOD";
  }
String json = "{";
if(!isnan(lastTemp))
  json += "\"temp\":" + String(lastTemp,1);
else
  json += "\"temp\":null";
if(!isnan(lastHum))
  json += ",\"hum\":" + String(lastHum,1);
else
  json += ",\"hum\":null";
if(!isnan(lastPressure))
  json += ",\"pres\":" + String(lastPressure,0);
else
  json += ",\"pres\":null";
if(!isnan(lastGas))
  json += ",\"gas\":" + String(lastGas,0);
else
  json += ",\"gas\":null";
json += ",\"tstat\":\"" + String(tStat) +
        "\",\"hstat\":\"" + String(hStat) +
        "\",\"air\":\"" + String(airStat) + "\"}";
server.send(200, "application/json", json);
}
////////////////////////////////////////////////////////////
// SYSTEM INFO
float citesteVCCdbg(){
  pinMode(4,INPUT); delay(10);
  int raw=analogRead(4);
  float voltage=(raw/4095.0)*3.3*2.0;
  pinMode(4,OUTPUT); digitalWrite(4,HIGH);
  return voltage;
}
void showSystemInfo() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_CYAN, GC9107_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(0, 0);
  gfx->println(" === SYSTEM INFO ===\n");

  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" Chip: "); gfx->println(ESP.getChipModel());
  gfx->print(" CPU MHz: "); gfx->println(ESP.getCpuFreqMHz());
  gfx->print(" Free RAM: "); gfx->println(ESP.getFreeHeap());
  gfx->print(" Flash MB: "); gfx->println(ESP.getFlashChipSize()/(1024*1024));
  gfx->println(" Display: GC9107");
  
  // 🔹 DHT11 status
  String dhtStatus = "N/A";
  if(!isSHT31){
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if(!isnan(t) && !isnan(h)) dhtStatus = "OK";
  }
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" DHT11: ");
  if(dhtStatus == "OK") gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
  else gfx->setTextColor(GC9107_RED, GC9107_BLACK);
  gfx->println(dhtStatus);

  // 🔹 SHT31 status
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" SHT31: ");
  if(isSHT31) gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
  else gfx->setTextColor(GC9107_RED, GC9107_BLACK);
  gfx->println(isSHT31 ? "OK" : "N/A");

   // 🔹 BME680 status
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" BME680: ");

  // verificare reală
  bool bmeOk = false;

  if(isBME680){
    if(bme.performReading()){
      bmeOk = true;
    }
  }
if(bmeOk){
  gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
  gfx->println("OK");
} else {
  gfx->setTextColor(GC9107_RED, GC9107_BLACK);
  gfx->println("N/A");
}
  // 🔥 TEMPERATURA CPU
  float cpuTemp = temperatureRead();
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" CPU Temp: ");
  gfx->print(cpuTemp, 1);
  gfx->print(" C");
  if(cpuTemp > 60.0){
    gfx->setTextColor(GC9107_RED, GC9107_BLACK);
    gfx->print(" !");
  } else {
    gfx->setTextColor(GC9107_GREEN, GC9107_BLACK);
    gfx->print(" OK");
  }
  // reset culoare
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->println();
  // 🔹 Baterie
  gfx->setTextColor(GC9107_WHITE, GC9107_BLACK);
  gfx->print(" VCC dbg: ");
  gfx->print(citesteVCCdbg(), 2);
  gfx->println(" V");
}
////////////////////////////////////////////////////////////
// BUTOANE
void checkButtons(){
  if(digitalRead(BTN_IO0)==LOW){ 
    paginaCurenta=SYSTEM_INFO;
    showSystemInfo();
    delay(200);
  }
  if(digitalRead(BTN_IO47)==LOW){
    paginaCurenta=MAIN_UI;
    drawStaticUI();
    afiseazaDHT();
    delay(200);
  }
}
///////////////////////////////////////////////////////////
///////  RESET BME680
void resetareSenzorBME() {
  if (isBME680) {
    gfx->fillRect(0, 100, 128, 28, GC9107_RED);
    gfx->setCursor(5, 110);
    gfx->setTextColor(GC9107_WHITE);
    gfx->print("RESET BME...");
    bme.begin(); // Reinițializare hardware
    // Reconfigurare parametrii după reset
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); 
    delay(3000); 
    gfx->fillRect(0, 100, 128, 28, GC9107_BLACK);
  }
}
///////////////////////////////////////////////////////
// SETUP
void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,HIGH);
  pinMode(BTN_IO0,INPUT_PULLUP);
  pinMode(BTN_IO47,INPUT_PULLUP);
  SPI.begin(TFT_SCLK,-1,TFT_MOSI,TFT_CS);
  gfx->begin();
  drawStaticUI();
  Wire.begin(I2C_SDA,I2C_SCL);
  delay(100); // Mic delay pentru stabilitate I2C 
  if(bme.begin(0x76)){
    isBME680 = true;
	// Setează parametrii pentru BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C timp de 150ms
  }
  if(sht31.begin(0x44)) {
      isSHT31 = true;
  }
  if(!isBME680 && !isSHT31) {
      dht.begin(); // Doar dacă nu ai senzori I2C
  }  
  afiseazaDHT();   
  
  // WiFi AP + Captive Portal
  WiFi.mode(WIFI_AP);
  IPAddress local_ip(192,168,110,1), gateway(192,168,110,1), subnet(255,255,255,0);
  WiFi.softAPConfig(local_ip,gateway,subnet);
  WiFi.softAP(ssidCurent.c_str(), apPassword);
  dnsServer.start(DNS_PORT,"*",local_ip);
  server.on("/",handleRoot);
  server.on("/data",handleData);
  server.on("/generate_204",handleRoot);
  server.on("/fwlink",handleRoot);
  server.on("/hotspot-detect.html",handleRoot);
  server.on("/connecttest.txt",handleRoot);
  server.onNotFound(handleRoot);
  server.begin();
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
}
////////////////////////////////////////////////////////////
// LOOP
void loop() {
  server.handleClient();
  checkButtons();
  if (digitalRead(BTN_IO0) == LOW) { // Dacă apeși butonul IO0
  resetareSenzorBME();
  delay(3000); // Debounce
}
  if(paginaCurenta == MAIN_UI){
    // 🔥 setare viteză blink în funcție de stare
    if(isnan(lastTemp) || isnan(lastHum)){
      blinkInterval = 200; // eroare → rapid
    }
    else if(lastTemp < 18 || lastTemp > 26 || lastHum < 30 || lastHum > 60){
      blinkInterval = 400; // alertă → mediu
    }
    else{
      blinkInterval = 1000; // normal → lent
    }
    // 🔥 blink controlat corect
    if(millis() - lastBlinkTime > blinkInterval){
      blinkState = !blinkState;
      lastBlinkTime = millis();
      afiseazaDHT(); // redraw doar la blink
    }
    // 🔹 update periodic senzori (fără blink)
    if(millis() - lastDHTUpdate > 5000){
      afiseazaDHT();
      lastDHTUpdate = millis();
    }
  }
  static unsigned long lastSensorCheck = 0;
  if(millis() - lastSensorCheck > 3000){
    checkSensors();
    lastSensorCheck = millis();
  }
}
