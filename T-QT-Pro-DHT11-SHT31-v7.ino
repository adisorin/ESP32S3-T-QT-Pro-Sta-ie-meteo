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

//////////////////////////////
// SSID DINAMIC
String ssidCurent = "ESP32S3-T/H-senzor";
unsigned long lastSSIDUpdate = 0;
const char* apPassword = "12345678"; // minim 8 caractere
//////////////////////////////
// DATE SENZOR (pentru web)
float lastTemp = 0;
float lastHum = 0;

//////////////////////////////
// TIMERE
unsigned long lastDHTUpdate = 0;

//////////////////////////////
// PAGINA ACTUALA
enum Pagina { MAIN_UI, SYSTEM_INFO };
Pagina paginaCurenta = MAIN_UI;

//////////////////////////////
// BLINK ALERT
bool blinkState = false;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500; // ms pentru blink

////////////////////////////////////////////////////////////
// FUNCTII TFT
void drawStaticUI() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 1);
  gfx->println("ESP CLIMA");
}

void afiseazaDHT() {
  float temp, hum;

  // Citire de la senzorul activ
  if(isSHT31){ temp=sht31.readTemperature(); hum=sht31.readHumidity(); }
  else { temp=dht.readTemperature(); hum=dht.readHumidity(); }

  blinkState = !blinkState;
  gfx->fillRect(0, 50, 128, 55, GC9107_BLACK);

  // Verificare eroare senzor
  if(isnan(temp) || isnan(hum)){
    gfx->setTextSize(2);
    if(blinkState){ gfx->setCursor(20, 55); gfx->setTextColor(GC9107_RED); gfx->println("SHT ERR"); }
    else { gfx->setCursor(20, 75); gfx->setTextColor(GC9107_BLUE); gfx->println("DHT ERR"); }
    return;
  }

  lastTemp = temp;
  lastHum = hum;

  // Afișare temperatură și umiditate
  gfx->setTextSize(2);
  gfx->setCursor(27, 55);
  gfx->setTextColor(GC9107_GREEN);
  gfx->print(temp, 1); // temperatura

  // 🔹 Desenăm cercul mic ca simbol °
  // x+30 și y+3 ajustează poziția cercului față de text
  gfx->drawCircle(27 + 55, 52 + 3, 2, GC9107_GREEN);

  // 🔹 Afișăm litera C după cerc
  gfx->setCursor(27 + 60, 55);
  gfx->print("C");

  gfx->setCursor(37, 80);
  gfx->setTextColor(GC9107_CYAN);
  gfx->print(hum,0); 
  gfx->print(" %");

  // Avertizare temperatură/umiditate
  if((temp<18.0 || temp>26.0) && blinkState){ gfx->setCursor(100,55); gfx->setTextColor(GC9107_RED); gfx->print(" !"); }
  if((hum<30.0 || hum>60.0) && blinkState){ gfx->setCursor(100,80); gfx->setTextColor(GC9107_RED); gfx->print(" !"); }

  // Update SSID dinamic
  if(millis()-lastSSIDUpdate>5000){ updateSSID(temp,hum); lastSSIDUpdate=millis(); }
}

////////////////////////////////////////////////////////////
// SSID DINAMIC
void updateSSID(float temp, float hum) {
  char ssidNou[32];

  const char* tStat = (temp < 18) ? "COLD" : (temp > 26) ? "HOT" : "OK";
  const char* hStat = (hum < 30) ? "DRY" : (hum > 60) ? "WET" : "OK";

  snprintf(ssidNou, sizeof(ssidNou), "RLS 3.5:  %.1f *C %s   %.0f%% %s",
           temp, tStat, hum, hStat);

  // NU schimba SSID dacă există clienți conectați
  if (WiFi.softAPgetStationNum() > 0) return;

  if (ssidCurent != String(ssidNou)) {
    ssidCurent = String(ssidNou);

    WiFi.softAPdisconnect(true);
    delay(100);

    // 🔥 REAPLICĂ IP-ul !!!
    IPAddress local_ip(192,168,110,1);
    IPAddress gateway(192,168,110,1);
    IPAddress subnet(255,255,255,0);

    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

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
    temp.innerHTML = "🌡️ " + d.temp.toFixed(1) + " °C " + d.tstat;
    hum.innerHTML  = "💧 " + d.hum.toFixed(0) + " % " + d.hstat;

    if(d.tstat==="HOT") temp.style.color="red";
    else if(d.tstat==="COLD") temp.style.color="cyan";
    else temp.style.color="white";

    if(d.hstat==="WET") hum.style.color="#00ccff";
    else if(d.hstat==="DRY") hum.style.color="orange";
    else hum.style.color="white";

    t.push(d.temp); h.push(d.hum);
    if(t.length > MAX_POINTS){ t.shift(); h.shift(); }

    // 🔹 autoscroll doar dacă nu tragi
    if(!isDragging){
      scrollOffset = Math.max(0, t.length - VISIBLE_POINTS);
    }

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
  String json="{\"temp\":"+String(lastTemp,1)+",\"hum\":"+String(lastHum,0)+",\"tstat\":\""+String(tStat)+"\",\"hstat\":\""+String(hStat)+"\"}";
  server.send(200,"application/json",json);
}

////////////////////////////////////////////////////////////
// SYSTEM INFO
float citesteBaterie(){
  pinMode(4,INPUT); delay(10);
  int raw=analogRead(4);
  float voltage=(raw/4095.0)*3.3*2.0;
  pinMode(4,OUTPUT); digitalWrite(4,HIGH);
  return voltage;
}

void showSystemInfo() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(0, 0);
  gfx->println("=== SYSTEM INFO ===\n");

  gfx->setTextColor(GC9107_WHITE);
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

  gfx->setTextColor(GC9107_WHITE);
  gfx->print(" DHT11: ");
  if(dhtStatus == "OK") gfx->setTextColor(GC9107_GREEN);
  else gfx->setTextColor(GC9107_RED);
  gfx->println(dhtStatus);

  // 🔹 SHT31 status
  gfx->setTextColor(GC9107_WHITE);
  gfx->print(" SHT31: ");
  if(isSHT31) gfx->setTextColor(GC9107_GREEN);
  else gfx->setTextColor(GC9107_RED);
  gfx->println(isSHT31 ? "OK" : "N/A");

  // 🔹 Baterie
  gfx->setTextColor(GC9107_WHITE);
  gfx->print(" Battery: "); gfx->print(citesteBaterie(), 2); gfx->println(" V");
}

////////////////////////////////////////////////////////////
// BUTOANE
void checkButtons(){
  if(digitalRead(BTN_IO0)==LOW){ 
    Serial.println("BTN_IO0 apăsat!");
    paginaCurenta=SYSTEM_INFO;
    showSystemInfo();
    delay(200);
  }
  if(digitalRead(BTN_IO47)==LOW){
    Serial.println("BTN_IO47 apăsat!");
    paginaCurenta=MAIN_UI;
    drawStaticUI();
    delay(200);
  }
}

////////////////////////////////////////////////////////////
// SETUP
void setup(){
  Serial.begin(115200);

  pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,HIGH);
  pinMode(BTN_IO0,INPUT_PULLUP);
  pinMode(BTN_IO47,INPUT_PULLUP);
  SPI.begin(TFT_SCLK,-1,TFT_MOSI,TFT_CS);
  gfx->begin();
  drawStaticUI();

  Wire.begin(I2C_SDA,I2C_SCL);
  if(sht31.begin(0x44)) isSHT31=true;
  else dht.begin();

  // WiFi AP + Captive Portal
  WiFi.mode(WIFI_AP);
  IPAddress local_ip(192,168,110,2), gateway(192,168,110,1), subnet(255,255,255,0);
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
  dnsServer.processNextRequest();
  server.handleClient();

  checkButtons();

  unsigned long currentMillis = millis();

  // update blink la fiecare 500 ms
  if(currentMillis - lastBlinkTime > blinkInterval){
    blinkState = !blinkState;
    lastBlinkTime = currentMillis;
  }

  if(currentMillis - lastDHTUpdate > 5000){
    if(paginaCurenta == MAIN_UI) afiseazaDHT();
    lastDHTUpdate = currentMillis;
  }
}