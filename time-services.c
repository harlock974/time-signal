/*
time-services.c - part of timesignal, a JJY/MSF/WWVB/DCF77
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


#include "time-services.h"

// Similar to WWVB, JJY uses BCD, but usually has a zero bit between the digits.
// So let's call it 'padded' BCD.
static uint64_t to_padded5_bcd(int n)
	{
	return (((n / 100) % 10) << 10) | (((n / 10) % 10) << 5) | (n % 10);
	}

// Regular BCD for the year encoding.
static uint64_t to_bcd(int n)
	{
	return (((n / 100) % 10) << 8) | (((n / 10) % 10) << 4) | (n % 10);
	}

static uint64_t parity(uint64_t d, uint8_t from, uint8_t to_including)
	{
	uint8_t result = 0;
	for (int bit = from; bit <= to_including; ++bit)
		{
		if (d & (1LL << bit)) result++;
		}
	return result & 0x1;
	}
	
static uint64_t is_leap_year(int year)
	{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
	}

uint64_t prepareMinute(enum time_service service, time_t t)
	{
	struct tm breakdown, tomorrow;
	time_t tomorrow_t;
	uint64_t	time_bits = 0,  // All the unused bits are zero.
				a_bits = 0,		// MSF
				b_bits = 0;		// MSF
	
	switch(service)
		{
		case JJY:
			// The JJY format uses Bit-Bigendianess, so we'll start with the first
			// bit left in our integer in bit 59.
			localtime_r(&t, &breakdown);  // If in JP, this is Japan Standard Time
			time_bits |= to_padded5_bcd(breakdown.tm_min) << (59 - 8);
			time_bits |= to_padded5_bcd(breakdown.tm_hour) << (59 - 18);
			time_bits |= to_padded5_bcd(breakdown.tm_yday + 1) << (59 - 33);
			time_bits |= to_bcd(breakdown.tm_year % 100) << (59 - 48);
			time_bits |= to_bcd(breakdown.tm_wday) << (59 - 52);
			
			time_bits |= parity(time_bits, 59-18, 59-12) << (59 - 36);  // PA1
			time_bits |= parity(time_bits, 59-8, 59-1) << (59 - 37);    // PA2
			
			// There is a different 'service announcement' encoding in minute 15 and 45,
			// but let's just ignore that for now. Consumer clocks probably don't care.
			break;
			
		case DCF77:
			t += 60;  // We're sending the _upcoming_ minute.
			localtime_r(&t, &breakdown);
			// Little endian bits. So we store big-endian bits and start transmitting from bit 0
			time_bits |= (breakdown.tm_isdst ? 1 : 0) << 17;
			time_bits |= (breakdown.tm_isdst ? 0 : 1) << 18;
			time_bits |= (1<<20);  // start time bit.
			time_bits |= to_bcd(breakdown.tm_min)  << 21;
			time_bits |= to_bcd(breakdown.tm_hour) << 29;
			time_bits |= to_bcd(breakdown.tm_mday) << 36;
			time_bits |= to_bcd(breakdown.tm_wday ? breakdown.tm_wday : 7) << 42;
			time_bits |= to_bcd(breakdown.tm_mon + 1) << 45;
			time_bits |= to_bcd(breakdown.tm_year % 100) << 50;
			
			time_bits |= parity(time_bits, 21, 27) << 28;
			time_bits |= parity(time_bits, 29, 34) << 35;
			time_bits |= parity(time_bits, 36, 57) << 58;
			break;
		
		case WWVB:
			gmtime_r(&t, &breakdown);  // Time transmission is always in UTC.
			// The WWVB format uses Bit-Bigendianess, so we'll start with the first
			// bit left in our integer in bit 59.
			time_bits |= to_padded5_bcd(breakdown.tm_min) << (59 - 8);
			time_bits |= to_padded5_bcd(breakdown.tm_hour) << (59 - 18);
			time_bits |= to_padded5_bcd(breakdown.tm_yday + 1) << (59 - 33);
			time_bits |= to_padded5_bcd(breakdown.tm_year % 100) << (59 - 53);
			time_bits |= is_leap_year(breakdown.tm_year + 1900) << (59 - 55);
			
			// Need local time for now and tomorrow to determine DST status.
			localtime_r(&t, &breakdown);
			tomorrow_t = t + 86400;
			localtime_r(&tomorrow_t, &tomorrow);
			time_bits |= (tomorrow.tm_isdst ? 1 : 0) << (59 - 57);
			time_bits |= (breakdown.tm_isdst ? 1 : 0) << (59 - 58);
			break;
			
		case MSF:
			t += 60;  // We're sending the _upcoming_ minute.
			localtime_r(&t, &breakdown);  // Local time, e.g. British standard time.
			
			// The MSF format uses Bit-Bigendianess, so we'll start with the first
			// bit left in our integer in bit 59.
			
			a_bits |= to_bcd(breakdown.tm_year % 100) << (59 - 24);
			a_bits |= to_bcd(breakdown.tm_mon + 1) << (59 - 29);
			a_bits |= to_bcd(breakdown.tm_mday) << (59 - 35);
			a_bits |= to_bcd(breakdown.tm_wday) << (59 - 38);
			a_bits |= to_bcd(breakdown.tm_hour) << (59 - 44);
			a_bits |= to_bcd(breakdown.tm_min) << (59 - 51);
			
			// First couple of bits: DUT; not being set.
			// (59 - 53): summer time warning. Not set.
			b_bits |= (parity(a_bits, 59-24, 59-17)==0) << (59 - 54); // Year parity
			b_bits |= (parity(a_bits, 59-35, 59-25)==0) << (59 - 55); // Day parity
			b_bits |= (parity(a_bits, 59-38, 59-36)==0) << (59 - 56); // Weekday parity
			b_bits |= (parity(a_bits, 59-51, 59-39)==0) << (59 - 57); // Time parity
			b_bits |= breakdown.tm_isdst << (59 - 58);
			time_bits = a_bits | b_bits;
			break;
		
		default:
			fprintf(stderr,"Time service still not implemented.\n");
			exit(1);
		}
	
	return time_bits;
	}

int getModulationForSecond(enum time_service service,uint64_t time_bits,int sec)
	{
	bool bit;
	switch(service)
		{
		case JJY:
			if (sec == 0 || sec % 10 == 9 || sec > 59) return 200;
			bit = time_bits & (1LL << (59 - sec));
			return (bit ? 500 : 800);
			
		case DCF77:
			if (sec >= 59) return 0;
			bit = time_bits & (1LL << sec);
			return (bit ? 200 : 100);
			
		case WWVB:
			if (sec == 0 || sec % 10 == 9 || sec > 59) return 800;
			bit = time_bits & (1LL << (59 - sec));
			return (bit ? 500 : 200);
			
		case MSF:
			if (sec == 0) return 500;
			bit = time_bits & (1LL << (59 - sec));
			return 100+bit*100+((sec>52)&&(sec<59))*100;
			
		default:
		fprintf(stderr,"Time service still not implemented.\n");
		exit(1);
		}
	}
