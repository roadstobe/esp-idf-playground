# esp-idf-playground

Small ESP-IDF projects I write while learning embedded development on the ESP32-S3.
Each numbered folder is a standalone project — no shared components, so every one
can be read from top to bottom on its own.

## Hardware

An ESP32-S3 board, three LEDs with current-limiting resistors, and a push button.

| Signal | GPIO | Wiring |
| --- | --- | --- |
| Red LED | 14 | GPIO → resistor → LED → GND |
| Yellow LED | 13 | GPIO → resistor → LED → GND |
| Green LED | 12 | GPIO → resistor → LED → GND |
| Button | 7 | GPIO → button → GND (internal pull-up, active low) |

## Build and flash

Requires ESP-IDF v6.1. The target is pinned in each project's `sdkconfig.defaults`,
so there is no need to run `idf.py set-target`.

```sh
cd 01-gpio-blink
idf.py build
idf.py -p /dev/tty.usbmodemXXXX flash monitor
```

## Projects

### 01-gpio-blink

A traffic light that runs a fixed cycle: red → red+yellow → green → blinking green
→ yellow → red. One state machine, one output table, no input.

Topics: `gpio_config()`, driving several outputs from a pin table, `vTaskDelay()`,
and a `switch` state machine with no `default` branch so the compiler catches a
state that was added but never handled.

### 02-pedestrian-priority

The same light, plus a button a pedestrian presses to request the crossing. The
request does not interrupt the traffic immediately:

- cars keep green for at least 15 s, so a crowd of pedestrians cannot stall traffic;
- once that minimum is spent, a request is answered after 5 s, which gives drivers
  a warning instead of a sudden change.

Both rules come out of a single expression in `green_hold_ms()`, deliberately
computed in signed arithmetic because the remaining minimum is usually negative.

Topics: GPIO interrupts, debouncing in the ISR by timestamp, waking a task from an
ISR through a queue instead of polling a flag (a busy loop here starves the idle
task and the task watchdog reboots the chip), and splitting the lights and the
button into reusable modules that return `esp_err_t` instead of aborting.

The button module (`main/button.c`) takes up to four buttons and reports which pin
was pressed, so the same code serves a second button without changes.
