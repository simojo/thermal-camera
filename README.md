# Thermal camera based on Raspberry Pi Pico and MLX90640

![Assembled Board](https://github.com/simojo/thermal-camera/blob/master/doc/assembly.jpeg)

## Overview

This camera was a dream I've had for a while to use up some parts I had lying
around on my bench. I took an embedded system design course in Fall 2025 under
Linden McClure, and I used it as an opportunity to prototype a design using wire
wrapping. The code worked, and I could use it as a thermal camera, but in April
2026, I finally taped it out to a PCB which is conducive to an enclosure and
being more rugged. I wrote the library for displaying to the thin-film
transistor display manually as the key effort to the project.

## User features

* 7 FPS refresh rate
* Dynamic color mapping
* Real-time temperature readout (min/max)
* USB-C charging
* 3.7V Lithium polymer/ion battery

## Technical Features

* CMake based project
* KiCad based layout
* Supports charging over battery
* MCP73831 charger with charging/full indicators
* 3.3V logic and power
