# ESP32S3 T-QT-Pro Statie meteo.
![20260328_094313](https://github.com/user-attachments/assets/69ca6b0b-4a11-4b86-a2df-0b82cedec61f)

# T-QT-Pro-DHT11-SHT31-v5
Fără internet și oră. 
![20260402_115254](https://github.com/user-attachments/assets/9790fe6d-fdee-49cb-b160-9c89b46e55c1)

# T-QT-Pro-DHT11-SHT31-v7
<img width="506" height="895" alt="image" src="https://github.com/user-attachments/assets/3e678dee-e149-4e61-ad97-49256762d0ee" />





# 📊 Prezentare proiect – Stație meteo cu ESP32 T-QT Pro

## 🔹 Introducere
Acest proiect utilizează placa **ESP32 T-QT Pro** pentru a crea o mini stație meteo inteligentă, capabilă să afișeze în timp real:
* temperatura 🌡️
* umiditatea 💧
* ora exactă 🕒
* nivelul semnalului WiFi 📶
Datele sunt afișate pe un ecran TFT de 128x128 pixeli.
---
### Hardware:
* ESP32 T-QT Pro
* Display TFT GC9107 (128x128)
* Senzor temperatură și umiditate:
  * DHT11 temperature and humidity sensor
  * SHT31 temperature and humidity sensor
* Conexiune WiFi

### Software / Biblioteci:
* Arduino IDE
* Arduino_GFX_Library
* WiFi + WiFiMulti
* DHT library
* Adafruit SHT31
* NTP (Network Time Protocol)
---
### 📡 Conectare WiFi automată
* Se încearcă conectarea la mai multe rețele salvate
* Afișează statusul pe ecran:
  * „Connecting WiFi…”
  * „WiFi Connected”
  * sau eroare („WRONG WIFI”)
---
### 🕒 Sincronizare automată a orei
* Ora este preluată de pe serverul NTP (`pool.ntp.org`)
* Se aplică fusul orar pentru România (EET/EEST)
* Afișare în format: `HH:MM:SS`
---
### 🌡️ Citire temperatură și umiditate
* Detectare automată a senzorului:
  * Dacă există SHT31 → îl folosește
  * Dacă nu → folosește DHT11

#### ✔️ Afișare:
* Temperatură în °C
* Umiditate în %

#### ⚠️ Alerte:
* Temperatură:
  * <18°C sau >26°C → avertizare
    
* Umiditate:
  * <30% sau >60% → avertizare
---
### 📶 Indicator semnal WiFi
* Afișează nivelul semnalului în 5 bare
* 
* Calcul bazat pe RSSI:
  * semnal puternic → 5 bare
  * semnal slab → 1 bară
---
### ❌ Gestionare erori senzori

* Dacă citirea e invalidă:
  * afișează alternativ:
    * „SHT ERR”
    * „DHT ERR”

---

## 🔹 Interfața grafică

Ecranul este organizat astfel:
* Titlu: „Temp & Humid”
* Centru: temperatură + umiditate
* Jos: ceas digital
* Dreapta sus: semnal WiFi

Culori utilizate:
* Verde → valori OK
* Roșu → avertizare
* Cyan → umiditate
* Galben → ceas
---
## 🔹 Structura programului

### setup()
* Inițializează:
  * ecranul TFT
  * senzorii
  * WiFi
* Detectează automat senzorul activ
---
### loop()

Rulează periodic:
* la 10 sec → verifică WiFi
* la 1 sec → actualizează ceasul
* la 2 sec → citește senzorii
---
## 🔹 Avantajele proiectului

✔️ Detectare automată senzor

✔️ Interfață grafică clară

✔️ Consum redus (fără delay excesiv)

✔️ Conectivitate WiFi

✔️ Extensibil (poți adăuga IoT, MQTT, etc.)

---
## 🔹 Posibile îmbunătățiri
* Trimitere date în cloud (ex: Thingspeak, MQTT)
* Aplicație mobilă
* Istoric temperatură
* Alertă prin notificări
* Baterie + mod low power
---
Autor: Sorinescu Adrian
