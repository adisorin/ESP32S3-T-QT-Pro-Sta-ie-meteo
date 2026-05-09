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



# ESP32 Climate Station + Web Dashboard

## Platformă IoT Open Source pentru monitorizare ambientală și interfață web interactivă

Arduino IDE
Espressif Systems
Arduino_GFX_Library

Acest proiect transformă un microcontroller ESP32-S3 într-o stație climatică inteligentă, cu:

* afișaj TFT color GC9107
* server web integrat
* hotspot WiFi autonom
* captive portal
* senzori multipli de temperatură / umiditate / presiune / calitate aer
* jocuri web integrate
* monitorizare live prin browser
* interfață optimizată pentru telefon mobil

Proiectul este construit pentru:

* dezvoltatori embedded
* makers
* pasionați IoT
* automatizări smart home
* dashboard-uri locale fără cloud

---

# Ce face proiectul

## Monitorizare climatică în timp real

Sistemul citește și afișează:

* temperatură
* umiditate
* presiune atmosferică
* rezistență gaz / calitate aer
* temperatură CPU ESP32
* tensiune alimentare

Compatibil cu:

* DHT11
* SHT31
* BME680

Sistemul detectează automat senzorii disponibili și schimbă modul de funcționare fără restart.

---

# Arhitectura proiectului

## Hardware

### Microcontroller

* ESP32-S3

### Display

* TFT GC9107 128x128

### Comunicare

* SPI pentru display
* I2C pentru senzori
* WiFi Access Point + STA

### Senzori

* DHT11
* SHT31
* BME680

---

# Funcționalități principale

## 1. UI TFT în timp real

Display-ul afișează:

* temperatură
* umiditate
* presiune atmosferică
* calitate aer
* stări de alertă
* erori senzori
* sistem info

Sistemul evită flicker-ul prin redraw parțial și update inteligent.

---

## 2. Captive Portal WiFi

ESP32 creează propriul hotspot:

```txt
MY HOME: 24°C OK 45% H OK
```

SSID-ul se actualizează dinamic folosind datele senzorilor.

Avantaje:

* vezi temperatura direct din lista WiFi
* acces fără aplicație
* acces instant din browser

---

## 3. Web Dashboard Responsive

Interfața web include:

* valori live
* grafic temperatură / umiditate
* clasificare meteo
* analiză presiune
* interpretare calitate aer
* update automat AJAX
* suport touch/mobile

---

## 4. Jocuri integrate

### Snake

* control touch
* control tastatură
* animație fluidă

### Minesweeper

* grid dinamic
* flood reveal
* flag system
* verificare win/lose

---

## 5. Digital Clock sincronizat NTP

ESP32:

* se conectează la router
* sincronizează timpul prin NTP
* afișează ceas live în browser

---

# Caracteristici software avansate

## Auto-detect senzori

Codul verifică permanent:

* dacă senzorii apar/dispar
* dacă există erori de comunicație
* dacă trebuie fallback pe alt senzor

---

## Sistem de alerte inteligente

Alertă pentru:

* temperatură prea mare/mică
* umiditate necorespunzătoare
* erori hardware
* calitate aer periculoasă

Blink-ul are viteză adaptivă:

* normal
* warning
* critical

---

## Corecție atmosferică

Presiunea este corectată în funcție de altitudine:

P=P_0\left(1-\frac{h}{44330}\right)^{-5.255}

Permite calibrare pentru diferite locații.

---

# Endpoint-uri API

## `/data`

Returnează:

```json
{
  "temp": 24.5,
  "hum": 45.2,
  "pres": 1018.3,
  "gas": 180.2
}
```

Perfect pentru:

* Home Assistant
* Node-RED
* Grafana
* aplicații mobile
* integrare MQTT

---

## `/system-data`

Expose:

* RAM
* CPU
* Flash
* senzori activi
* clienți conectați
* temperatură CPU

---

# Tehnologii utilizate

## Backend Embedded

* C++
* Arduino Framework
* ESP32 SDK

## Frontend

* HTML5
* CSS3
* Vanilla JavaScript
* Canvas API

## Librării

* WiFi.h
* WebServer.h
* Arduino_GFX_Library
* Adafruit_BME680
* Adafruit_SHT31

---

# Optimizări importante

## Performanță TFT

* redraw local
* fără refresh complet
* consum redus CPU

## Consum memorie

* HTML servit din PROGMEM style strings
* fără framework-uri grele

## Stabilitate

* watchdog logic
* reconnect senzori
* fallback automat

---

# De ce este interesant pentru dezvoltatori

Acest proiect demonstrează cum un ESP32 poate deveni:

* server web autonom
* sistem embedded realtime
* UI device
* hotspot inteligent
* mini platformă IoT
* sistem multimedia lightweight

Totul fără:

* Raspberry Pi
* Linux
* cloud
* backend extern

---

# Posibile extensii

## Smart Home

* relee
* automatizări HVAC
* ventilare inteligentă

## IoT

* MQTT
* Home Assistant
* Grafana
* InfluxDB

## UI

* dark/light themes
* WebSocket live
* PWA installable

## Hardware

* baterie Li-Ion
* solar charging
* senzori CO2
* touchscreen

---

# Cui se adresează

* programatori embedded
* dezvoltatori IoT
* pasionați ESP32
* makers
* hobby electronics
* smart home builders

---

# Licență

Proiectul folosește licența:

MIT License

Permite:

* utilizare comercială
* modificare
* distribuție
* integrare în alte produse

---

# Concluzie

Acest proiect este mai mult decât un simplu termometru IoT.

Este o demonstrație completă de:

* embedded systems
* web development
* realtime UI
* captive portal networking
* senzori inteligenți
* optimizare hardware/software

Totul rulând pe un singur ESP32-S3.

Autor: Sorinescu Adrian
