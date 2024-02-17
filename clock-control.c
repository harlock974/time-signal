#include "clock-control.h"

// -- Implementation for Raspberry Pi Series --

// Raspberry 1 and 2 have different base addresses for the periphery
#define BCM2708_PERI_BASE        0x20000000
#define BCM2709_PERI_BASE        0x3F000000
#define BCM2711_PERI_BASE        0xFE000000

#define GPIO_REGISTER_OFFSET     0x00200000
#define CLOCK_REGISTER_OFFSET    0x00101000

#define REGISTER_BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x).
#define INP_GPIO(g)  *(gpio_port+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)  *(gpio_port+((g)/10)) |=  (1<<(((g)%10)*3))
#define ALT0_GPIO(g) *(gpio_port+((g)/10)) |=  (4<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

// Clock control
#define CLK_PASSWD  (0x5A << 24)
#define CLK_CTL_MASH(x) ((x)<<9)
#define CLK_CTL_BUSY    (1<<7)
#define CLK_CTL_KILL    (1<<5)
#define CLK_CTL_ENAB    (1<<4)
#define CLK_CTL_SRC(x)  ((x)<<0)

#define CLK_DIV_DIVI(x) ((x)<<12)
#define CLK_DIV_DIVF(x) ((x)<< 0)

#define CLK_CMGP0_CTL 28
#define CLK_CMGP0_DIV 29

volatile uint32_t *gpio_port;
volatile uint32_t *gpio_set_bits;
volatile uint32_t *gpio_clr_bits;
volatile uint32_t *clk;

enum RaspberryPiModel
	{
	PI_MODEL_1,
	PI_MODEL_2,
	PI_MODEL_3,
	PI_MODEL_4
	};

enum RaspberryPiModel piModel;

void EnableClockOutput(bool on)
	{
	if (on) ALT0_GPIO(4);  // Pinmux GPIO4 into outputting clock.
	else INP_GPIO(4);
	}

void StopClock()
	{
	*(clk + CLK_CMGP0_CTL) = CLK_PASSWD | CLK_CTL_KILL;
	// Wait until clock confirms not to be busy anymore.
	while (clk[CLK_CMGP0_CTL] & CLK_CTL_BUSY) usleep(10);
	EnableClockOutput(false);
	}

// BCM2835-ARM-Peripherals.pdf, page 105 onwards.
double StartClock(double requested_freq)
	{
	// Figure out best clock source to get closest to the requested
	// frequency with MASH=1. We check starting from the highest frequency to
	// find lowest jitter opportunity first.
	
	struct { int src; double frequency; } kClockSources[] =
		{
		 { 7, piModel==PI_MODEL_4 ?  0     : 216.0e6 },	// HDMI  <- this can be problematic if monitor connected
		 { 1, piModel==PI_MODEL_4 ? 54.0e6 :  19.2e6 },	// regular oscillator
		};
		
	int divI = -1;
	int divF = -1;
	int best_clock_source = -1;
	double smallest_error_so_far = 1e9;
	printf("Clock sources:\n");
	for (size_t i = 0; i < sizeof(kClockSources)/sizeof(kClockSources[0]); ++i)
		{
		printf("%d : ",kClockSources[i].src);
		double division = kClockSources[i].frequency / requested_freq;
		if (division < 2 || division > 4095)
			{
			puts("");
			continue;
			}
		int test_divi = (int) division;
		int test_divf = (division - test_divi) * 1024;
		double freq = kClockSources[i].frequency / (test_divi + test_divf/1024.0);
		double error = fabsl(requested_freq - freq);
		//printf("Clock source : %d freq : %.4f error : %.4f\n",kClockSources[i].src,freq,error);
		printf("freq : %.4f error : %.4f\n",freq,error);
		if (error >= smallest_error_so_far) continue;
		smallest_error_so_far = error;
		best_clock_source = i;
		divI = test_divi;
		divF = test_divf;
		}
	
	if (divI < 0) return -1.0;  // Couldn't find any suitable clock.
	
	StopClock();
	
	const uint32_t ctl = CLK_CMGP0_CTL;
	const uint32_t div = CLK_CMGP0_DIV;
	const uint32_t src = kClockSources[best_clock_source].src;
	const uint32_t mash = 1;  // Good approximation, low jitter.
	
	clk[div] = CLK_PASSWD | CLK_DIV_DIVI(divI) | CLK_DIV_DIVF(divF);
	usleep(10);
	clk[ctl] = CLK_PASSWD | CLK_CTL_MASH(mash) | CLK_CTL_SRC(src);
	usleep(10);
	
	clk[ctl] |= CLK_PASSWD | CLK_CTL_ENAB;
	
	//EnableClockOutput(true);
	
	// There have been reports of different clock source frequencies. This
	// helps figuring out which source was picked.
	fprintf(stderr, "\nChoose clock %d at %gHz / %.3f = %.3f\n\n",
		kClockSources[best_clock_source].src,
		kClockSources[best_clock_source].frequency,
		divI + divF/1024.0,
		kClockSources[best_clock_source].frequency / (divI + divF/1024.0));
	return kClockSources[best_clock_source].frequency / (divI + divF/1024.0);
	}

enum RaspberryPiModel GetPiModel()
	{
	FILE *f;
	unsigned int pi_revision;
	char *lineptr;
	size_t n=0;
	f=fopen("/proc/cpuinfo","r");
	if (f != NULL)
		{
		while (getline (&lineptr, &n, f) != -1)
			{
			if (strstr(lineptr, "Revision"))
				{
				sscanf(lineptr, "Revision : %x", &pi_revision);
				unsigned int pi_type = (pi_revision >> 4) & 0xff;
				printf("Pi type : 0x%02x\n",pi_type);
				switch (pi_type)
					{
					case 0x00: /* A */
					case 0x01: /* B, Compute Module 1 */
					case 0x02: /* A+ */
					case 0x03: /* B+ */
					case 0x05: /* Alpha ?*/
					case 0x06: /* Compute Module1 */
					case 0x09: /* Zero */
					case 0x0c: /* Zero W */
						return PI_MODEL_1;
					
					case 0x04:  /* Pi 2 */
					case 0x12:  /* Zero W 2 (behaves close to Pi 2) */
						return PI_MODEL_2;
					
					case 0x11: /* Pi 4 */
						// A first test did not seem to work. Maybe the registers changed ?
						fprintf(stderr, "Note: Frequency generation is known to not work on Pi4; "
						"Use older Pis for now.\n");
						return PI_MODEL_4;
					}
				}
			}
		}
	return PI_MODEL_3;
	}

static uint32_t *mmap_bcm_register(off_t register_offset)
	{
	off_t base = BCM2709_PERI_BASE;  // safe fallback guess.
	switch (piModel)
		{
		case PI_MODEL_1: base = BCM2708_PERI_BASE; break;
		case PI_MODEL_2: base = BCM2709_PERI_BASE; break;
		case PI_MODEL_3: base = BCM2709_PERI_BASE; break;
		case PI_MODEL_4: base = BCM2711_PERI_BASE; break;
		}
	
	int mem_fd;
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
		{
		perror("can't open /dev/mem: ");
		return NULL;
		}
	
	uint32_t *result = (uint32_t*) mmap
				(
				NULL,               // Any adddress in our space will do
				REGISTER_BLOCK_SIZE,   // Map length
				PROT_READ|PROT_WRITE,  // Enable r/w on GPIO registers.
				MAP_SHARED,
				mem_fd,                // File to map
				base + register_offset // Offset to bcm register
				);
				
	close(mem_fd);
	
	if (result == MAP_FAILED)
		{
		perror("mmap error: ");
		fprintf(stderr, "MMapping from base 0x%lx, offset 0x%lx\n",	base, register_offset);
		return NULL;
		}
	return result;
	}

bool GPIO_init()
	{
	piModel = GetPiModel();
	gpio_port = mmap_bcm_register(GPIO_REGISTER_OFFSET);
	if (gpio_port == NULL)
		{
		fprintf(stderr, "Need to be root\n");
		return false;
		}
	gpio_set_bits = gpio_port + (0x1C / sizeof(uint32_t));
	gpio_clr_bits = gpio_port + (0x28 / sizeof(uint32_t));
	clk = mmap_bcm_register(CLOCK_REGISTER_OFFSET);
	return gpio_port != MAP_FAILED && clk != MAP_FAILED;
	}
