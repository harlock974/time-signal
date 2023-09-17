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

