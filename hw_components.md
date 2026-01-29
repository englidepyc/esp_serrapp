# Sensors
- AHT20 (I2C) x5
- Soil moisture capacitive sensor (analog AOUT) x5
- float switch (interrupt)
- Fire infrared photo diode
# Actuators
(4 relays)
- Water pump
- Fan (to extract humidity)
- Heater (connected to the single SSR relay)
- Humidifier (on/off analog)
# Utilities
- TCA9548A (I2C mux) 8 uscite selezionabili con i2c
- CD74HC4067 (Analog mux) 16 uscite selezionabili coi pin S0-S3 (no i2c)
- io expander PCF8574 8 canali
- SN74HC595N (shift register) (rip ✝)
# ESP32-S3 pinout
0-3,14,19,20,41,42,45-48,21 (free)
38,39,40 (sd card)
35,36,37 (psram)
4-13, 15-18 (camera)

# Wirings
# 🥭 Smart Mango Garden - Hardware Pinout & Wiring

**Target Board:** ESP32-S3-WROOM-1 (N16R8) Camera Board
**Strategy:** Expansion Bus via Shift Register (74HC595) + Shared I2C

---

## 1. ESP32-S3 GPIO Map (The "Golden Map")

These are the physical connections between the components and the ESP32 headers.

| Component | Signal | GPIO Pin | Function / Notes |
| :--- | :--- | :--- | :--- |
| **Shift Register (74HC595)** | **SER (Data)** | **GPIO 47** | Feeds data to the register. |
| **Shift Register (74HC595)** | **SRCLK (Clock)** | **GPIO 48** | Shifts bits into storage. |
| **Shift Register (74HC595)** | **RCLK (Latch)** | **GPIO 14** | Pushes stored bits to physical output pins. |
| **I2C Bus (Sensors)** | **SDA** | **GPIO 41** | Connect to TCA9548A SDA. |
| **I2C Bus (Sensors)** | **SCL** | **GPIO 42** | Connect to TCA9548A SCL. |
| **Analog Mux (CD74HC4067)**| **SIG (Signal)** | **GPIO 2** | **ADC1_CH1**. Reads moisture values. |
| **Float Switch** | **Digital In** | **GPIO 21** | **RTC Pin**. Wakes ESP32 on Low Water. |
| **Fire Sensor** | **Digital In** | **GPIO 45** | **Emergency Interrupt**. (Active LOW). |
| **Power Supply** | **5V / VBUS** | **5V Pin** | Powers Relays & Shift Register. |
| **Ground** | **GND** | **GND** | Common Ground (CRITICAL). |

> **⚠️ WARNING:** Do NOT use GPIO 38, 39, 40 (SD Card) or 19, 20 (USB) for these connections.

---

## 2. Shift Register Output Map (74HC595)

We use the Shift Register to control actuators AND select sensors. The ESP32 sends a single "Byte" (8 bits) to update everything at once.

| Bit Index | Chip Pin | Function | Wiring Destination |
| :--- | :--- | :--- | :--- |
| **Bit 0 (Q0)** | Pin 15 | **Pump Relay** | Relay IN1 |
| **Bit 1 (Q1)** | Pin 1 | **Fan Relay** | Relay IN2 |
| **Bit 2 (Q2)** | Pin 2 | **Heater (SSR)** | SSR Control + |
| **Bit 3 (Q3)** | Pin 3 | **Humidifier** | Relay IN4 |
| **Bit 4 (Q4)** | Pin 4 | **Mux S0** | CD74HC4067 Pin S0 |
| **Bit 5 (Q5)** | Pin 5 | **Mux S1** | CD74HC4067 Pin S1 |
| **Bit 6 (Q6)** | Pin 6 | **Mux S2** | CD74HC4067 Pin S2 |
| **Bit 7 (Q7)** | Pin 7 | *Unused* | (Available for future expansion) |

---

## 3. Detailed Component Wiring

### A. Analog Multiplexer (CD74HC4067)
*Used to read 5 Soil Moisture Sensors using only 1 ESP32 Analog Pin.*

* **VCC:** 3.3V
* **GND:** GND
* **EN (Enable):** GND (Always On)
* **SIG (Common):** Connect to **ESP32 GPIO 2**
* **S0, S1, S2:** Connect to **Shift Register Q4, Q5, Q6** (See table above)
* **S3:** Connect to GND (We only need channels 0-7)
* **C0 - C4:** Connect to the Analog Out pin of your 5 Soil Sensors.

### B. I2C Multiplexer (TCA9548A)
*Used to talk to 5 identical AHT20 sensors.*

* **VIN:** 3.3V
* **GND:** GND
* **SDA:** Connect to **ESP32 GPIO 41**
* **SCL:** Connect to **ESP32 GPIO 42**
* **SC0 / SD0:** AHT20 Sensor #1
* **SC1 / SD1:** AHT20 Sensor #2
* **...**
* **SC4 / SD4:** AHT20 Sensor #5

### C. Shift Register (74HC595)
*The "IO Expander".*

* **Pin 16 (VCC):** 3.3V (or 5V if Relays require 5V logic, but 3.3V is safer for ESP32)
* **Pin 8 (GND):** GND
* **Pin 10 (SRCLR):** 3.3V (Keep High to prevent resetting)
* **Pin 13 (OE):** GND (Keep Low to enable outputs)
* **Pin 14 (SER):** **ESP32 GPIO 47**
* **Pin 11 (SRCLK):** **ESP32 GPIO 48**
* **Pin 12 (RCLK):** **ESP32 GPIO 14**

