# PICO Development Kit

## Overview
This is the complete demo code for PICO Development Kit with 3.5-inch TFT capacitive touch screen.

## Features
* **Hardware Demo Mode**
  - RGB LED color wheel control
  - Buzzer on/off
  - Button-controlled LED (with debounce)
  - Real-time joystick position display
  
* **Calculator Application**
  - Support for basic arithmetic operations (addition, subtraction, multiplication, division)
  - Decimal point operations
  - Smart result display

* **System Functions**
  - Based on FreeRTOS real-time operating system
  - LVGL graphical interface
  - Dual-core task scheduling

## Hardware Specifications

### Display Parameters
* Resolution: 320x480 pixels
* Display Driver IC: ST7796U
* Operating Voltage: 3.3V 
* Touch Type: Capacitive Touch Screen (GT911)
* Display Communication Protocol: SPI (SPI0)
* Touch Screen Communication Protocol: I2C (I2C0 SDA: GP8, SCL: GP9)

### Pin Definitions
| Component | Pin |
|---|---|
| Buzzer | GP13 |
| LEDs | D1: GP16, D2: GP17, D3: 3V3, D4: 5V |
| RGB LED | GP12 |
| Joystick | X-axis: ADC0 (GP26), Y-axis: ADC1 (GP27) |
| Buttons | BTN1: GP15, BTN2: GP14 |

### TFT Display Pinout
| Raspberry Pi Pico | 3.5" TFT Screen |
|---|---|
| GP2 | CLK |
| GP3 | MOSI |
| GP4 | MISO |
| GP5 | CS |
| GP6 | DC |
| GP7 | RST |
| GP10 | TPRST |
| GP11 | TPINT |

### Capacitive Touch Screen Pinout
| Raspberry Pi Pico | Capacitive Touch Screen |
|---|---|
| I2C0 SDA GP8 | SDA |
| I2C0 SCL GP9 | SCL |

Online Tutorial: www.readthedocs.com