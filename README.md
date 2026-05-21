# Ruddler

> Rudder pedal controller using load cells and Arduino Micro — with a real-time web dashboard.

[![Deploy Dashboard](https://github.com/willianszwy/ruddler/actions/workflows/deploy.yml/badge.svg)](https://github.com/willianszwy/ruddler/actions/workflows/deploy.yml)
[![Dashboard](https://img.shields.io/badge/dashboard-online-58a6ff?style=flat)](https://willianszwy.github.io/ruddler)

---

## Overview

Ruddler turns two load cells into a USB rudder pedal that works with any flight simulator (DCS, MSFS, X-Plane). Instead of potentiometers, it measures actual force — giving precise, wear-free input with configurable response.

```
[Left Pedal] ── [Load Cell 1] ──┐
                                 ├── HX711 ── Arduino Micro ── USB ── PC
[Right Pedal] ─ [Load Cell 2] ──┘
```

---

## Features

- Force-based input — no pots, no wear
- Real-time web dashboard (Chrome / Edge)
- Configurable force max, dead zone and smoothing
- Settings saved to EEPROM (persist after reboot)
- Hardware tare button
- Works as a native USB HID joystick

---

## Hardware

| Component | Qty |
|---|---|
| Arduino Micro (ATmega32U4) | 1 |
| HX711 breakout (CJMCU) | 1 |
| Load cell 3-wire half-bridge | 2 |
| Resistor 1kΩ (optional) | 2 |
| Momentary button | 1 |

---

## Wiring

### Load Cells → HX711

Two half-bridge load cells combined into a full Wheatstone bridge:

```
Load Cell 1          Load Cell 2
───────────          ───────────
RED ───────┬──────── RED
           └──────────────────── E+  (HX711)

BLACK ─────┬──────── BLACK
           └──────────────────── E-  (HX711)

YELLOW ──────────────────────── A+  (HX711)

                    YELLOW ───── A-  (HX711)
```

### HX711 → Arduino Micro

| HX711 | Arduino Micro |
|---|---|
| VCC | VCC (5V) |
| GND | GND |
| DT  | Pin 4 |
| SCK | Pin 5 |

### Tare Button

Connect a momentary button between **Pin 10** and **GND**.

---

## Firmware

### Requirements

Install via Arduino Library Manager:

- [HX711_ADC by Olav Kallhovd](https://github.com/olkal/HX711_ADC)
- [Joystick by Matthew Heironimus](https://github.com/MHeironimus/ArduinoJoystickLibrary)

### Setup

1. Open `firmware/ruddler/ruddler.ino` in Arduino IDE
2. Select **Arduino Micro** as the board
3. Upload

### Calibration

1. Upload the firmware with no weight on the load cells
2. Open Serial Monitor at **115200 baud**
3. Place a known weight (e.g. 500g) and note the raw value
4. Calculate: `scale_factor = raw_value ÷ weight_in_grams`
5. Update `scale.set_scale(YOUR_FACTOR)` in the code

---

## Dashboard

**[willianszwy.github.io/ruddler](https://willianszwy.github.io/ruddler)**

> Requires Chrome or Edge (Web Serial API). No installation needed — runs entirely in the browser.

```
┌─────────────────────────────────────────────┐
│  Ruddler                                    │
│  Dashboard de configuração e monitoramento  │
├─────────────────────────────────────────────┤
│  [ Connect to Arduino ]                     │
├─────────────────────────────────────────────┤
│  Rudder Position                            │
│  ←──────────────●──────────────→           │
│    Left        Center        Right          │
├──────────┬──────────┬──────────┬────────────┤
│ Force(g) │   Axis   │    %     │  Rate(Hz)  │
│   1240   │  -16382  │  -50%    │   80 Hz    │
├─────────────────────────────────────────────┤
│  History  ╭─────────────────────────────╮  │
│           │    ∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿         │  │
│           ╰─────────────────────────────╯  │
├─────────────────────────────────────────────┤
│  Settings                                   │
│  Force Max  ────────────●──  15000 g        │
│  Dead Zone  ──●─────────── 150 g            │
│  EMA Alpha  ─────●──────── 0.20             │
├─────────────────────────────────────────────┤
│  [ Zero (Tare) ]     [ Reset Config ]       │
└─────────────────────────────────────────────┘
```

### Usage

1. Plug in the Arduino Micro
2. Open the dashboard and click **Connect**
3. Select the Arduino COM port
4. Adjust settings in real time — they save automatically to EEPROM

### Parameters

| Parameter | Default | Description |
|---|---|---|
| Force Max | 15000 g | Force required to reach 100% axis deflection |
| Dead Zone | 150 g | Center dead zone to prevent drift |
| EMA Alpha | 0.20 | Smoothing factor (0.1 = smooth, 0.5 = responsive) |

> Settings are stored in EEPROM — the Arduino loads them automatically on every boot, no dashboard connection required.

---

## Simulator Setup (DCS)

1. Go to **Options → Controls → Axis Commands**
2. Find **Rudder Axis** and click **Add**
3. Press one pedal to assign the axis
4. Open **Axis Tune** and calibrate if needed

---

## Project Structure

```
ruddler/
├── firmware/
│   └── ruddler/
│       └── ruddler.ino       Arduino sketch
├── dashboard/
│   └── index.html            Web dashboard
└── .github/
    └── workflows/
        └── deploy.yml        GitHub Pages auto-deploy
```

---

## License

MIT
