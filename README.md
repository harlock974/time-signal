# time-signal
A JJY/MSF/WWVB/DCF77 time signal radio transmitter for Raspberry Pi

**time-signal** allows to convert a Raspberry Pi to a radio time server using using either JJY, MSF, WWVB or DCF77 protocol, allowing to automatically adjust time on radio-controlled watch or clock. 

## Platforms

**time-signal** has been succesfully tested on Raspberry Pi 3 Model B Rev 1.2 and Zero W.

## Dependencies

**time-signal** relies on [Wiring Pi](http://wiringpi.com/) library, which should be pre-installed with standard Raspbian systems. If not (e.g. on Rasbian Lite), do

```
sudo apt-get install wiringpi
```

## Build

Inside the directory where the source files are, simply type `make`

## Usage

```
sudo ./time-signal [OPTIONS]
```
### Options

-s <service> :  Time service: either off DCF77, WWVB, JJY40, JJY60, MSF

-v : Verbose. Show modulation enveloppe.

-c : Send carrier wave only, without time signal. Usefull for testing frequencies.

-h : This help.

## Hardware

**time-signal** uses GPIO 4 (pin 7) to transmit the modulated carrier wave. The simplest hardware you can use to transmit the signal is to connect a piezo speaker between GPIO 4 and a ground pin (for example pin 6). Check on datasheet if your piezo speaker could be directly connected to GPIO, as litterature on this subject is contradictory. In doubt, use a 1k resistor and a flyback diode :

What is fun with piezo speaker is that you actually hear the modulated signal. The range is limited, around 2 cm max. As these devices are conceived for audible sound, they work better with JJY 40 kHz signal, which is the lowest frquency time service.
