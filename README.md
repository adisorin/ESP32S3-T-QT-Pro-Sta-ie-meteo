# ESP32S3 T-QT-Pro Statie meteo.
![20260328_094313](https://github.com/user-attachments/assets/69ca6b0b-4a11-4b86-a2df-0b82cedec61f)

# T-QT-Pro-DHT11-SHT31-v5
Fără internet și oră. 
![20260402_115254](https://github.com/user-attachments/assets/9790fe6d-fdee-49cb-b160-9c89b46e55c1)

![WhatsApp Image 2026-04-04 at 14 56 00](https://github.com/user-attachments/assets/dd79836f-1468-43c6-85a1-d9c8fccac1d2)


# T-QT-Pro-DHT11-SHT31-v7
<img width="506" height="895" alt="image" src="https://github.com/user-attachments/assets/3e678dee-e149-4e61-ad97-49256762d0ee" />

![Captură de ecran 2026-04-12 165227](https://github.com/user-attachments/assets/5fddd20e-10c9-483c-91d3-71788858a5a0)

![WhatsApp Image 2026-04-12 at 17 03 45](https://github.com/user-attachments/assets/a4f63304-40db-42c4-b9e5-f2ecb313c5cc)

![WhatsApp Image 2026-04-12 at 17 03 45-1](https://github.com/user-attachments/assets/679c7ad0-97f1-4175-b033-780ae78b54da)

![WhatsApp Image 2026-04-12 at 17 03 46](https://github.com/user-attachments/assets/5cec1dfa-3822-49d3-9c22-d2f6a64ef989)

![WhatsApp Image 2026-04-12 at 17 03 46-2](https://github.com/user-attachments/assets/bb9bd1e1-aca1-4355-834a-809695e4836f)

![WhatsApp Image 2026-04-12 at 17 03 46-3](https://github.com/user-attachments/assets/480e628b-97ab-4649-81b8-ada616ede618)



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
