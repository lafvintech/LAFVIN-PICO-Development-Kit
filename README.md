# PICO Development Kit

## Overview
This is the complete demo code for the PICO Development Kit with a 3.5-inch TFT capacitive touch screen.

The project currently provides build presets for **Pico W / Pico WH** and **Pico 2 W**. The Pico W UF2 has also been tested on the original Pico hardware.

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

## Build

### Prerequisites

Install and configure the Raspberry Pi Pico SDK toolchain first. Pico 2 W builds require Pico SDK 2.1.0 or later.

Clone submodules before building:

```bash
git submodule update --init --recursive
```

This project uses two FreeRTOS kernel checkouts:

| Board preset | FreeRTOS path |
|---|---|
| `pico-w` | `components/FreeRTOS` |
| `pico2-w` | `components/FreeRTOS-RaspberryPi` |

### Pico W / Pico WH

```bash
cmake --preset pico-w
cmake --build --preset pico-w -j8
```

The UF2 is generated under:

```text
build/pico_w/hello_world.uf2
```

This UF2 can also be used on the original Pico hardware in the tested kit setup.

### Pico 2 W

```bash
cmake --preset pico2-w
cmake --build --preset pico2-w -j8
```

The UF2 is generated under:

```text
build/pico2_w/hello_world.uf2
```

The `pico2-w` preset enables active-low button handling for this kit.

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

Online Tutorial: https://lafvin-pico-development-kit.readthedocs.io/en/latest
