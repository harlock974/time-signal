/*
time-services.h - part of timesignal, a JJY/MSF/WWVB/DCF77
radio transmitter for Raspberry Pi
Copyright (C) 2023 Pierre Brial <p.brial@tethys.re>

Parts of this code is based on txtempus code written by Henner Zeller
Source: https://github.com/hzeller/txtempus
Copyright (C) 2018 Henner Zeller <h.zeller@acm.org>
Licensed under the GNU General Public License, version 3 or later

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

enum time_service {JJY, DCF77, WWVB, MSF};

uint64_t prepareMinute(enum time_service service, time_t t);

int getModulationForSecond(enum time_service service,uint64_t time_bits,int sec);
