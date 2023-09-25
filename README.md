# time-signal
A multiband time signal radio transmitter for Raspberry Pi

**time-signal** allows to convert a Raspberry Pi to a radio time server using using either JJY, MSF, WWVB or DCF77 protocol, allowing to automatically adjust time on radio-controlled watch or clock. 

## Platforms

**time-signal** has been succesfully tested on :
* Raspberry Pi Zero W
* Raspberry Pi 3 Model B Rev 1.2
* Raspberry Pi 4 Model B Rev 1.1

## Dependencies

**time-signal** relies on [Wiring Pi](http://wiringpi.com/) library, which should be pre-installed with standard Raspbian systems. If not (e.g. on Rasbian Lite), do :

```
sudo apt-get install wiringpi
```

For Raspberry Pi 4, do :

```
cd /tmp
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb
```

## Build

Inside the directory where the source files are, simply type `make`

## Usage

```
sudo ./time-signal [OPTIONS]
```
### Options

-s <service> :  Time service: either of DCF77, WWVB, JJY40, JJY60, MSF

-v : Verbose. Show modulation enveloppe.

-c : Send carrier wave only, without time signal. Usefull for testing frequencies.

-h : This help.

## Hardware

**time-signal** uses GPIO 4 (pin 7) to transmit the modulated carrier wave and a Ground pin. 5V power pin could also be used by your circuit if you want to amplify the signal.

![Capture d’écran_2023-09-25_21-27-04](https://github.com/harlock974/time-signal/assets/6268242/d27c548d-a9a9-4dd3-8360-b7247b49799a)
_[Raspberry Pi pinout](https://pinout.xyz)_

The simplest hardware you can use to transmit the signal is to connect a coil and a 220Ω resistor between GPIO4 and ground :

![IMG_20230925_212405](https://github.com/harlock974/time-signal/assets/6268242/79d53d74-a45c-4ef1-8484-cf3a85fff586)

Here with a 60cm wire and 14 turns of 13mm diameter, the range reaches 10cm.

For a longer range and still simple circuit, you can use a ferrite antenna salvaged from an AM receiver with a N mosfet amplifier. You'll then have this 3 components circuit :

![MOSFET_OSCILLATOR_JJY40 EMF](https://github.com/harlock974/time-signal/assets/6268242/0602c816-f4b7-4955-aaed-0eb1e36e3022)

![DSC_4398s](https://github.com/harlock974/time-signal/assets/6268242/3a3ce337-c3cd-41d2-af07-a76bfd2c779e)

_The hairy 3 components circuit_

If you don't have a mosfet, you can use a NPN transistor instead with a 4.7K resistor between its base and GPIO4.

Capacitor value depends on antenna inductance and target frequency : `C = 1 / (4×π²×L×f²)`

AM antennae have usually an inductance between 500 and 700 µH, so the theoretical C values are :
* 40000 Hz : 26 nF
* 60000 Hz : 12 nF
* 77500 Hz :  7 nF

As small mosfets like BS270 can handle up to 250 mA, you can design a more powerful transmitter with two complementary mosfets as a NOT gate and a lower value resistor :

![DUAL-MOSFET_OSCILLATOR_JJY40s](https://github.com/harlock974/time-signal/assets/6268242/39b1b727-c7c0-4f64-bfb6-f22c3791dee8)

I successfully tested these circuits with JJY40 et DCF77 signals and reached a range of ten meters with JJY40 and the dual mosfet configuration.

![DSC_4399s](https://github.com/harlock974/time-signal/assets/6268242/9069a2a0-c241-44fa-8030-728f90dce124)

_G-Shock strong reception (L3)_
