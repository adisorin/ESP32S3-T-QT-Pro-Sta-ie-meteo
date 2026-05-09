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
bool dhtOK = false;
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
float altitudini[] = {97.0, 132.0}; //Remetea Mare, Lugoj
int indexAltitudine = 1; // 132 default Lugoj
float altitudineCurenta = altitudini[indexAltitudine];
//////////////////////////////
// TIMERE
unsigned long lastDHTUpdate = 0;
unsigned long lastBME680Update = 0;

//////////////////////////////
//CLIENTI
volatile int webClients = 0;
unsigned long lastPing = 0;
int activeClients = 0;


/////////////////////////////////
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
      lastPressure = corectiePresiune(presAbs, altitudineCurenta) + 0.3; // corecție nivel mare + etajul 3 Lugoj(132m). RemeteaM(97m)

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

    dhtOK = !isnan(temp) && !isnan(hum);

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
    if (lastGas <= 650 && lastGas >= 150) {
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
  // =========================
  // ❗ BLINK ALERT - RECONSTRUITĂ
  // =========================
  
  // Alertă Temperatură (Poziția 100, 35)
  if (temp < 18.0 || temp > 26.0) {
    gfx->setCursor(100, 35);
    if (blinkState) {
      gfx->setTextColor(GC9107_RED, GC9107_BLACK);
      gfx->print(" !");
    } else {
      // Ștergere prin dreptunghi negru pentru siguranță
      gfx->fillRect(100, 35, 25, 16, GC9107_BLACK); 
    }
  } else {
    // Dacă temperatura e OK, șterge zona definitiv
    gfx->fillRect(100, 35, 25, 16, GC9107_BLACK);
  }

  // Alertă Umiditate (Poziția 100, 60)
  if (hum < 30.0 || hum > 60.0) {
    gfx->setCursor(100, 60);
    if (blinkState) {
      gfx->setTextColor(GC9107_RED, GC9107_BLACK);
      gfx->print(" !");
    } else {
      // Ștergere prin dreptunghi negru
      gfx->fillRect(100, 60, 25, 16, GC9107_BLACK);
    }
  } else {
    // Dacă umiditatea e OK, șterge zona definitiv
    gfx->fillRect(100, 60, 25, 16, GC9107_BLACK);
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
  snprintf(ssidNou, sizeof(ssidNou), "MY HOME:  %.0f *C %s   %.0f%% H %s",
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

.chart-wrapper {
  position: relative;
  display: flex;
  justify-content: center;
  align-items: center;
  margin: 10px 0;
  width: 100%;
}

.side-btn {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  padding: 6px 8px;
  background: #222;
  color: white;
  border: 1px solid #444;
  border-radius: 8px;
  cursor: pointer;
  font-size: 11px;
  z-index: 10;
}

.side-btn.left {
  left: 6px;   /* lipit de canvas */
}

.side-btn.right {
  right: 6px;  /* lipit de canvas */
}

.side-btn:hover {
  background: #444;
}

.nav-buttons {
  display: flex;
  justify-content: space-between;
  margin: 10px 20px;
}

.nav-buttons button {
  padding: 8px 12px;
  background: #222;
  color: white;
  border: 1px solid #444;
  border-radius: 10px;
  box-shadow: 0 0 18px #00ffcc;
  cursor: pointer;
}

.nav-buttons button:hover {
  background: #444;
}

</style>
</head>
<body>
<h2>💧🌡️ MY HOUSE 🌡️💧</h2>
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
<div class="chart-wrapper">
  <canvas id="chart" width="335" height="150"></canvas>
</div>

<div class="nav-buttons">
  <button onclick="goSystem()">⚙️ System</button>
  <button onclick="goClock()" style="box-shadow: 0 0 18px #00ffcc;">🕒 Clock</button> 
  <button onclick="goGame()" style="box-shadow: 0 0 18px #ff00ff;">🐍 Snake</button> 
  <button onclick="goMines()" style="box-shadow: 0 0 18px #ffff00;">💣 Mines</button> 
</div>

<script>

function goGame() { window.location.href = "/game"; }

function goMines() { window.location.href = "/minesweeper"; }

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
  fetch('/ping');
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
    } else if (d.gas <= 650) {
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
    // Presiune ridicată (Anticiclon - Vreme foarte frumoasă)
    if(d.pres >= 1020) pres.style.color = "#00BFFF"; // Deep Sky Blue (un albastru viu)

    // Presiune peste medie (Vreme stabilă)
    else if(d.pres >= 1013) pres.style.color = "#00FF99"; // Spring Green (verde)

    // Presiune normală/ușor scăzută (Vreme variabilă)
    else if(d.pres >= 1005) pres.style.color = "#FFD700"; // Gold (galben - atenție la schimbare)

    // Presiune scăzută (Ciclon - Posibile precipitații)
    else if(d.pres >= 995) pres.style.color = "#FF8C00"; // Dark Orange (instabilitate)

    // Presiune foarte scăzută (Furtună/Alertă)
    else pres.style.color = "#FF0000"; // Red (Cod roșu de furtună)


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
setInterval(upd, 5000);
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
function goSystem() {
  window.location.href = "/system";
}

function goMain() {
  window.location.href = "/";
}

function goClock() { window.location.href = "/clock"; }
</script>
</body>
</html>
)rawliteral";
  server.send(200,"text/html",html);
}


void handleSystemPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
body { 
  font-family: Arial; 
  background:#111; 
  color:white; 
  text-align:center; 
  margin:0;
  padding:0;
}

h2 {
  font-size: 42px;
  margin-top: 25px;
}

.card { 
  margin:25px; 
  padding:40px;
  background:#222; 
  border-radius:18px;
  font-size: 32px;
  box-shadow: 0 0 18px #00ffcc;
  line-height: 2;
}

.nav-buttons {
  margin-top: 25px;
}

.nav-buttons button {
  font-size: 30px;
  padding: 16px 26px;
  background: #222;
  color: white;
  border: 2px solid #444;
  border-radius: 14px;
  box-shadow: 0 0 18px #00ffcc;
  cursor: pointer;
}

.nav-buttons button:hover {
  background: #333; /* Un gri mai deschis decât originalul #222 */
}

.nav-buttons button:active {
  transform: scale(0.97);
  background: #444;
}

@media (max-width: 480px) {
  h2 { font-size: 48px; }
  .card { font-size: 36px; padding: 45px; }
  .nav-buttons button {
    font-size: 34px;
    padding: 18px 30px;
  }
}

/* Culorile cerute */
.status-ok { color: #00FF00; font-weight: bold; }
.status-na { color: #FF0000; font-weight: bold; }
</style>
</head>
<body>

<h2>⚙️ SYSTEM INFO</h2>

<div class="card" id="info">Loading...</div>

<div class="nav-buttons">
  <button onclick="goMain()">🏠 Main</button>
</div>

<script>
function getSt(val) {
  let cl = (val === "OK") ? "status-ok" : "status-na";
  return "<span class='" + cl + "'>" + val + "</span>";
}

function load() {
  fetch('/system-data')
    .then(r => r.json())
    .then(d => {
      document.getElementById("info").innerHTML =
        "Chip: " + d.chip + "<br>" +
        "CPU: " + d.cpu_mhz + " MHz<br>" +
        "Heap: " + d.heap + "<br>" +
        "Flash: " + d.flash + " MB<br>" +
        "CPU Temp: " + d.cpu_temp.toFixed(0) + " °C<br>" +
        "Clients: " + d.clients + "<br>" +
        "BME 680: " + getSt(d.bme) + "<br>" +
        "SHT 31: " + getSt(d.sht) + "<br>" +
        "DHT 11: " + getSt(d.dht) + "<br>" +
        "VCC dbg: " + d.vcc_dbg.toFixed(2) + " V";
    });
}

setInterval(load, 2000);
load();

function goMain() {
  window.location.href = "/";
}
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleGamePage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 Snake</title>
<style>
  body { background: #111; color: white; text-align: center; font-family: sans-serif; margin: 0; overflow: hidden; touch-action: none; }
  h2 { margin: 10px 0; color: #ff00ff; text-shadow: 0 0 10px #ff00ff; font-size: 20px; }
  
  /* Container Canvas */
  #game-layer { display: flex; flex-direction: column; align-items: center; }
  canvas { border: 3px solid #ff00ff; background: #000; box-shadow: 0 0 20px #ff00ff; max-width: 90vw; }

  /* Butoane Control - Fără Emoji, folosim Border-Arrows */
  .controls { display: grid; grid-template-columns: repeat(3, 80px); grid-template-rows: repeat(2, 70px); gap: 10px; margin-top: 15px; justify-content: center; }
  .btn { 
    background: #222; border: 2px solid #00ffcc; border-radius: 12px; 
    display: flex; align-items: center; justify-content: center; 
    box-shadow: 0 0 10px #00ffcc; cursor: pointer; -webkit-tap-highlight-color: transparent;
  }
  .btn:active { background: #00ffcc; transform: scale(0.9); }

  /* Desenăm săgețile folosind CSS pur (triunghiuri) */
  .arrow { width: 0; height: 0; border-style: solid; }
  .up { border-width: 0 15px 20px 15px; border-color: transparent transparent #00ffcc transparent; }
  .down { border-width: 20px 15px 0 15px; border-color: #00ffcc transparent transparent transparent; }
  .left { border-width: 15px 20px 15px 0; border-color: transparent #00ffcc transparent transparent; }
  .right { border-width: 15px 0 15px 20px; border-color: transparent transparent transparent #00ffcc; }

  .btn:active .arrow { border-color: black transparent black transparent; }
  .btn:active .up { border-bottom-color: black; }
  .btn:active .down { border-top-color: black; }
  .btn:active .left { border-right-color: black; }
  .btn:active .right { border-left-color: black; }

  .b-up { grid-column: 2; }
  .b-left { grid-column: 1; grid-row: 2; }
  .b-down { grid-column: 2; grid-row: 2; }
  .b-right { grid-column: 3; grid-row: 2; }

  .btn-back { margin-top: 20px; padding: 10px; background: none; border: 1px solid #444; color: #888; border-radius: 5px; cursor: pointer; }
</style>
</head>
<body>
  <div id="game-layer">
    <h2>SNAKE GAME</h2>
    <canvas id="snakeGame" width="300" height="300"></canvas>
    
    <div class="controls">
      <div class="btn b-up" onclick="changeDir('UP')"><div class="arrow up"></div></div>
      <div class="btn b-left" onclick="changeDir('LEFT')"><div class="arrow left"></div></div>
      <div class="btn b-down" onclick="changeDir('DOWN')"><div class="arrow down"></div></div>
      <div class="btn b-right" onclick="changeDir('RIGHT')"><div class="arrow right"></div></div>
    </div>
    
    <button class="btn-back" onclick="window.location.href='/'">BACK TO SENSORS</button>
  </div>

<script>
  const canvas = document.getElementById("snakeGame");
  const ctx = canvas.getContext("2d");
  const box = 20;
  let score = 0;
  let currentDir = ""; 
  let nextDir = ""; // Stocăm direcția imediat ce e apăsată
  let gameStarted = false;
  
  let snake = [{x: 7 * box, y: 7 * box}];
  let food = { x: Math.floor(Math.random() * 14 + 1) * box, y: Math.floor(Math.random() * 14 + 1) * box };

  function changeDir(dir) {
    if(!gameStarted) gameStarted = true;
    
    // Validăm direcția instantaneu pentru a preveni întoarcerea la 180 grade
    if(dir=="LEFT" && currentDir!="RIGHT") nextDir="LEFT";
    else if(dir=="UP" && currentDir!="DOWN") nextDir="UP";
    else if(dir=="RIGHT" && currentDir!="LEFT") nextDir="RIGHT";
    else if(dir=="DOWN" && currentDir!="UP") nextDir="DOWN";
  }

  // Control tastatură (fără lag)
  document.addEventListener("keydown", e => {
    let key = e.keyCode;
    if(key==37) changeDir("LEFT");
    else if(key==38) changeDir("UP");
    else if(key==39) changeDir("RIGHT");
    else if(key==40) changeDir("DOWN");
  });

  function draw() {
    ctx.fillStyle = "black";
    ctx.fillRect(0,0,300,300);

    // Mâncare
    ctx.fillStyle = "red";
    ctx.beginPath();
    ctx.arc(food.x + box/2, food.y + box/2, box/2 - 2, 0, 2 * Math.PI);
    ctx.fill();

    // Șarpe
    for(let i=0; i<snake.length; i++) {
      ctx.fillStyle = (i==0) ? "#ff00ff" : "#00ffcc";
      ctx.fillRect(snake[i].x, snake[i].y, box-1, box-1);
    }

    if(!gameStarted) {
      ctx.fillStyle = "white";
      ctx.font = "16px Arial";
      ctx.fillText("APASA SAGEATA PENTRU START", 35, 150);
      return;
    }

    // Actualizăm direcția curentă cu cea salvată din input
    currentDir = nextDir;

    let headX = snake[0].x;
    let headY = snake[0].y;

    if(currentDir == "LEFT") headX -= box;
    if(currentDir == "UP") headY -= box;
    if(currentDir == "RIGHT") headX += box;
    if(currentDir == "DOWN") headY += box;

    // Verificăm dacă a mâncat
    if(headX == food.x && headY == food.y) {
      score++;
      food = { x: Math.floor(Math.random() * 14 + 1) * box, y: Math.floor(Math.random() * 14 + 1) * box };
    } else {
      snake.pop();
    }

    let newHead = { x: headX, y: headY };

    // Coliziuni
    if(headX < 0 || headY < 0 || headX >= 300 || headY >= 300 || collision(newHead, snake)) {
      clearInterval(game);
      alert("GAME OVER! Scor: " + score);
      location.reload();
    }

    snake.unshift(newHead);
  }

  function collision(head, array) {
    for(let i=0; i<array.length; i++) if(head.x == array[i].x && head.y == array[i].y) return true;
    return false;
  }

  // 250ms pentru viteză mică, dar input-ul e captat separat prin changeDir
  let game = setInterval(draw, 600); 
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}


void handleMinesweeper() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 Minesweeper</title>
<style>
  body { background: #111; color: white; text-align: center; font-family: sans-serif; margin: 0; }
  h2 { margin: 15px 0; color: #ffff00; text-shadow: 0 0 10px #ffff00; }
  #grid { 
    display: grid; grid-template-columns: repeat(10, 30px); gap: 2px; 
    justify-content: center; background: #333; padding: 5px; border-radius: 8px;
    margin: 0 auto; width: fit-content; box-shadow: 0 0 20px #ffff00;
  }
  .cell { 
    width: 30px; height: 30px; background: #444; display: flex; 
    align-items: center; justify-content: center; font-weight: bold; 
    cursor: pointer; user-select: none; border-radius: 2px;
  }
  .cell.revealed { background: #222; cursor: default; }
  .cell.mine { background: #ff0000; }
  .btn-back { margin: 20px; padding: 10px 20px; background: #222; border: 1px solid #444; color: #888; border-radius: 8px; cursor: pointer; }
  .info { margin-bottom: 10px; font-size: 1.2em; }
</style>
</head>
<body>
  <h2>MINESWEEPER</h2>
  <div class="info">Mines: <span id="mineCount">10</span></div>
  <div id="grid"></div>
  <button class="btn-back" onclick="window.location.href='/'">BACK TO SENSORS</button>

<script>
  const size = 10;
  const minesNum = 10;
  let board = [];
  const grid = document.getElementById('grid');

  function init() {
    board = Array(size).fill().map(() => Array(size).fill(0));
    let placed = 0;
    while(placed < minesNum) {
      let r = Math.floor(Math.random()*size), c = Math.floor(Math.random()*size);
      if(board[r][c] !== 'M') { board[r][c] = 'M'; placed++; }
    }
    for(let r=0; r<size; r++) {
      for(let c=0; c<size; c++) {
        if(board[r][c] === 'M') continue;
        let count = 0;
        for(let dr=-1; dr<=1; dr++) {
          for(let dc=-1; dc<=1; dc++) {
            if(r+dr>=0 && r+dr<size && c+dc>=0 && c+dc<size && board[r+dr][c+dc] === 'M') count++;
          }
        }
        board[r][c] = count;
      }
    }
    draw();
  }

  function draw() {
    grid.innerHTML = '';
    for(let r=0; r<size; r++) {
      for(let c=0; c<size; c++) {
        const cell = document.createElement('div');
        cell.className = 'cell';
        cell.dataset.r = r; cell.dataset.c = c;
        cell.onclick = () => reveal(r, c);
        // Long press pentru steag (simulat prin contextmenu)
        cell.oncontextmenu = (e) => { e.preventDefault(); cell.innerText = (cell.innerText==='🚩')?'':'🚩'; cell.style.color = '#ff00ff'; };
        grid.appendChild(cell);
      }
    }
  }

  function reveal(r, c) {
    const cell = grid.children[r * size + c];
    if(cell.classList.contains('revealed') || cell.innerText === '🚩') return;
    
    cell.classList.add('revealed');
    const val = board[r][c];
    
    if(val === 'M') {
      cell.innerText = '💣';
      cell.classList.add('mine');
      setTimeout(() => { alert('GAME OVER!'); init(); }, 100);
    } else {
      cell.innerText = val > 0 ? val : '';
      if(val === 1) cell.style.color = '#00ffcc';
      if(val === 2) cell.style.color = '#00ff00';
      if(val >= 3) cell.style.color = '#ff0000';
      
      if(val === 0) {
        for(let dr=-1; dr<=1; dr++) {
          for(let dc=-1; dc<=1; dc++) {
            if(r+dr>=0 && r+dr<size && c+dc>=0 && c+dc<size) reveal(r+dr, c+dc);
          }
        }
      }
    }
    checkWin();
  }

  function checkWin() {
    const revealedCount = document.querySelectorAll('.cell.revealed').length;
    if(revealedCount === size * size - minesNum) {
      alert('FELICITARI! Ai curatat zona!');
      init();
    }
  }

  init();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleClockPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Digital Clock</title>
<style>
  body { background: #000; color: #00ffcc; font-family: 'Courier New', monospace; text-align: center; display: flex; flex-direction: column; justify-content: center; height: 100vh; margin: 0; }
  .clock-container { border: 2px solid #00ffcc; padding: 20px; border-radius: 20px; display: inline-block; margin: auto; box-shadow: 0 0 20px #00ffcc; }
  #time { font-size: 60px; text-shadow: 0 0 20px #00ffcc; margin: 0; }
  #date { font-size: 24px; color: #008877; margin-top: 10px; }
  .btn-back { margin-top: 40px; padding: 15px 30px; background: transparent; border: 1px solid #008877; color: #008877; border-radius: 10px; cursor: pointer; font-size: 18px; text-decoration: none; display: inline-block; }
  .btn-back:hover { background: #00ffcc; color: #000; }
</style>
</head>
<body>
  <div class="clock-container">
    <div id="time">00:00:00</div>
    <div id="date">Așteptare sincronizare...</div>
    <a href="/" class="btn-back">ÎNAPOI LA SENZORI</a>
  </div>

<script>
  function updateClock() {
    const acum = new Date();
    const h = String(acum.getHours()).padStart(2, '0');
    const m = String(acum.getMinutes()).padStart(2, '0');
    const s = String(acum.getSeconds()).padStart(2, '0');
    const d = acum.toLocaleDateString('ro-RO', { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' });
    
    document.getElementById('time').textContent = h + ":" + m + ":" + s;
    document.getElementById('date').textContent = d;
  }
  setInterval(updateClock, 1000);
  updateClock();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
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
  json += ",\"pres\":" + String(lastPressure,1);
else
  json += ",\"pres\":null";
if(!isnan(lastGas))
  json += ",\"gas\":" + String(lastGas,1);
else
  json += ",\"gas\":null";
json += ",\"tstat\":\"" + String(tStat) +
        "\",\"hstat\":\"" + String(hStat) +
        "\",\"air\":\"" + String(airStat) + "\"}";
server.send(200, "application/json", json);
}

void handleSystemData() {
  String json = "{";
  json += "\"chip\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"cpu_mhz\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"flash\":" + String(ESP.getFlashChipSize()/(1024*1024)) + ",";
  json += "\"dht\":\"" + String(dhtOK ? "OK" : "N/A") + "\",";
  json += "\"sht\":\"" + String(isSHT31 ? "OK" : "N/A") + "\",";
  json += "\"bme\":\"" + String(isBME680 ? "OK" : "N/A") + "\",";
  json += "\"cpu_temp\":" + String(temperatureRead()) + ",";

  int totalClients = WiFi.softAPgetStationNum() + activeClients;
  json += "\"clients\":" + String(totalClients) + ",";
  
  // 🔥 ADAUGĂ AICI
  json += "\"vcc_dbg\":" + String(citesteVCCdbg(), 2);

  json += "}";
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
  gfx->print(cpuTemp, 0);
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
  static unsigned long pressStart47 = 0;

  // BTN IO0 (nemodificat)
  if(digitalRead(BTN_IO0)==LOW){ 
    paginaCurenta=SYSTEM_INFO;
    showSystemInfo();
    delay(200);
  }

  // BTN IO47
  if(digitalRead(BTN_IO47)==LOW){
    if(pressStart47 == 0){
      pressStart47 = millis();
    }

    // LONG PRESS > 1 sec → schimbă altitudine
    if(millis() - pressStart47 > 1000){
      schimbaAltitudine();
      pressStart47 = 0;
      delay(300);
    }
  } 
  else {
    // SHORT PRESS → funcția originală
    if(pressStart47 != 0 && millis() - pressStart47 < 1000){
      paginaCurenta=MAIN_UI;
      drawStaticUI();
      afiseazaDHT();
    }
    pressStart47 = 0;
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
void schimbaAltitudine() {

  indexAltitudine++;
  if(indexAltitudine >= (sizeof(altitudini)/sizeof(altitudini[0]))) {
    indexAltitudine = 0;
  }

  altitudineCurenta = altitudini[indexAltitudine];

  Serial.print("Altitudine: ");
  Serial.println(altitudineCurenta);

  // feedback pe display
  gfx->fillRect(0, 100, 128, 28, GC9107_BLUE);
  gfx->setCursor(5, 110);
  gfx->setTextColor(GC9107_WHITE);
  gfx->print("ALT: ");
  gfx->print(altitudineCurenta,0);
  gfx->print(" m");

  delay(1000);
  gfx->fillRect(0, 100, 128, 28, GC9107_BLACK);
}


void handlePing(){
  lastPing = millis();
  activeClients = 1;
  server.send(200, "text/plain", "OK");
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
  WiFi.mode(WIFI_AP_STA);
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
  server.on("/system-data", handleSystemData);
  server.on("/system", handleSystemPage);
  server.on("/game", handleGamePage); // Înregistrează pagina jocului
  server.on("/minesweeper", handleMinesweeper); // Noul joc
  server.on("/clock", handleClockPage);
  server.onNotFound(handleRoot);
  server.begin();
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  server.on("/ping", handlePing);


  const char* ssid = "%netrunower ";
  const char* password = "0511#A#b#cc";

  WiFi.begin(ssid, password);

  Serial.print("Conectare WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(7200, 3600, "pool.ntp.org"); // Pentru România (UTC+2 + DST)

  Serial.println("\nConectat la router!");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
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
  if(millis() - lastPing > 10000){
  activeClients = 0; // dacă nu mai face ping → considerat offline
}
}