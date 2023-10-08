/*
time-signal.c - a JJY/MSF/WWVB/DCF77 radio transmitter for Raspberry Pi
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

#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <wiringPi.h>
#include "time-services.h"

#define CLOCKPIN 7		// wiringpi pin's numbering scheme

int usage(const char *msg, const char *progname)
	{
	fprintf(stderr, "%susage: %s [options]\n"
		"Options:\n"
		"\t-s <service>          : Service; one of "
		"'DCF77', 'WWVB', 'JJY40', 'JJY60', 'MSF'\n"
		"\t-v                    : Verbose.\n"
		"\t-c                    : Carrier wave only.\n"
		"\t-h                    : This help.\n",
	msg, progname);
	return 1;
	}

void signaux(int sigtype)
	{
	switch(sigtype)
		{
		case SIGINT : printf ("\nSIGINT");break;
		case SIGTERM : printf ("\nSIGTERM");break;
		default : printf ("\nUnknow %d",sigtype);
		}
	pinMode (CLOCKPIN, OUTPUT);
	printf (" signal received - Programme terminé\n");
	exit(0);
	}

int main(int argc, char *argv[])
{
bool
	verbose = false,
	carrier_only = false;

int
	modulation,
	frequency=60000,
	opt;
	
uint64_t minute_bits;
char
	*time_source="",
	date_string[] = "1969-07-21 00:00:00",
	*lineptr;
size_t n=0;
enum time_service service;
time_t now,minute_start;
struct timespec target_wait;
struct tm tv;
FILE *f;

signal(SIGINT,signaux);
signal(SIGTERM,signaux);

puts("time-signal, a JJY/MSF/WWVB/DCF77 radio transmitter");
puts("Copyright (C) 2023 Pierre Brial");
puts("This program comes with ABSOLUTELY NO WARRANTY.");
puts("This is free software, and you are welcome to");
puts("redistribute it under certain conditions.\n");

while ((opt = getopt(argc, argv, "vs:hc")) != -1)
	{
	switch (opt)
		{
		case 'v':
			verbose = true;
		break;
		case 's':
			time_source = optarg;
		break;
		case 'c':
			carrier_only = true;
		break;
		default:
			return usage("", argv[0]);
		}
	}

if (strcasecmp(time_source, "DCF77") == 0)
	{
	frequency = 77500;
	service = DCF77;
	}
else if (strcasecmp(time_source, "WWVB") == 0) service = WWVB;
else if (strcasecmp(time_source, "JJY40") == 0)
	{
	frequency = 40000;
	service = JJY;
	}
else if (strcasecmp(time_source, "JJY60") == 0) service = JJY;
else if (strcasecmp(time_source, "MSF") == 0) service = MSF;
else return usage("Please choose a service name with -s option\n", argv[0]);

// Check if board is pi4
f=fopen("/proc/device-tree/model","r");
if (f != NULL)
		{
		getline (&lineptr, &n, f);
		if (verbose) puts(lineptr);
		if (strstr(lineptr, "Pi 4"))
			{
			//puts("Raspberry Pi 4 detected.\n");
			frequency = frequency*16/45; // Frequency correction for Pi4 oscillator (19.2 -> 54 Mhz)
			}
		fclose(f);
		}

wiringPiSetup ();
pinMode (CLOCKPIN, GPIO_CLOCK);
gpioClockSet(CLOCKPIN,frequency);
if (!carrier_only) pinMode (CLOCKPIN, OUTPUT);

// Give max priority to this programm
struct sched_param sp;
sp.sched_priority = 99;
sched_setscheduler(0, SCHED_FIFO, &sp);

now = time(NULL);
minute_start = now - now % 60; // round to minute

while(1)
	{
	if (carrier_only) continue;
	localtime_r(&minute_start, &tv);
  	strftime(date_string, sizeof(date_string), "%Y-%m-%d %H:%M:%S", &tv);
	printf("%s\n",date_string);
	minute_bits = prepareMinute(service,minute_start);

	for (int second = 0; second < 60; ++second)
		{
		modulation = getModulationForSecond(service,minute_bits,second);
			
		// First, let's wait until we reach the beginning of that second
		target_wait.tv_sec = minute_start + second;
		target_wait.tv_nsec = 0;
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target_wait, NULL);
		
		if (service == JJY) pinMode (CLOCKPIN, GPIO_CLOCK);	// Set signal to HIGH
		else pinMode (CLOCKPIN, OUTPUT);					// Set signal to LOW
		
		if (verbose)
			{
			fprintf(stderr,"%03d ",modulation);
			if ((second+1)%15 == 0) fprintf(stderr,"\n");
			}
		
		target_wait.tv_nsec = modulation*1e6;
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target_wait, NULL);
		
		if (service == JJY) pinMode (CLOCKPIN, OUTPUT);	// Set signal to LOW
		else pinMode (CLOCKPIN, GPIO_CLOCK);			// Set signal to HIGH
		}
		
	minute_start += 60;
	}
}	
