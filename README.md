# Hive Mind: Technical Challenge Scoreboard

## Project Overview
This project is an interactive LED scoreboard for a Bee-Themed Game Show. It uses an Arduino to control 4 quadrants of LEDs, read IR beam sensors, and respond to a host's remote control.

**Target Audience:** Middle School Students & DI Appraisers.

## Hardware Layout

### 1. LED Matrix (The "Honeycomb")
* **Structure:** 4 Quadrants.
* **Orientation:** The LED data wire starts at the **Bottom-Left** corner of each quadrant.
* **Wiring Pattern:** Serpentine (Zig-Zag).
    * Row 0 (Bottom): Left -> Right
    * Row 1: Right -> Left
    * Row 2: Left -> Right
* **Pins:** 6, 7, 8, 9 (defined in `config.h`).

### 2. Sensors & Controls
* **IR Beams:** 4 sensors (one per quadrant) to detect "scores" (balls/objects passing through).
    * Pins: 2, 3, 4, 5.
* **IR Receiver:** Reads signals from the Host's remote.
    * Pin: 11.

## How to Use the Remote
* **CH-**: Emergency Reset / Go to Intro Mode.
* **1**: Start **Round 1** (Filling the honeycomb).
* **2**: Start **Round 2** (Not yet implemented).
* **3**: Start **Round 3** (Not yet implemented).
* **4**: Start **Round 4** (Not yet implemented).
* **5 / CH+**: **Finale** (Winner animation).

## Software Design (For the Programmers)
* **No Classes:** We use simple functions so the code is easy to read.
* **Modules:** The code is split into `.h` files based on what they do (e.g., `leds.h` handles lights, `beams.h` handles sensors).
* **The Loop:**
    1.  Read the Remote.
    2.  Check the current "Mode" (Intro, Round 1, etc.).
    3.  Update the LEDs **only if the remote isn't talking** (prevents flickering).

## Round Logic
* **Intro:** Displays a waiting animation.
* **Round 1:** When a beam is broken, the "honey" level in that quadrant rises from the bottom.
* **Finale:** Turns all LEDs green to celebrate.