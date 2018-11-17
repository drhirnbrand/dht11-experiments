#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <errno.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <pigpio.h>

/**
 * Being a Java Developer, I have no clue on how to write C Code.
 * 
 */
int main(int argc, char *argv[]) {

	if (gpioInitialise() < 0) {
		printf("Failed to initialize GPIO!\n");
		exit(1);
	}

	int gpio = 4;
	int r = 0;

	float temperature;
	float humidity;

	r = setup(gpio);
	if (r != 0) {
		if (r == -1) {
		printf("Timeout");
		}
		exit(1);
	}

	r = poll(gpio, &humidity, &temperature);

	if (r != 0) {
		exit(1);
	}

	printf("Humidity %f, Temperature %f", humidity, temperature);

	return 0;
}

/**
 * Read the 40 Pulses from the Wire and convert it to float.
 */
int poll(int gpio, float* humidity, float* temperature) {
	int r = 0;

	int DHT_PULSES = 41;
	int pulseCounts[DHT_PULSES * 2];
	int DHT_MAXCOUNT = 32000;

	// We are counting low->high and high->low transitions, and measure the time between edges.
	for (int i = 0; i < (DHT_PULSES * 2); i = i + 2) {
		int lc=0;
		int hc=0;

		while (gpioRead(gpio) == 0) {
			lc++;
			if (lc > DHT_MAXCOUNT) {
				printf("Timeout LOW Pulse %d", i);
				return -1;
			}
		}

		while (gpioRead(gpio) == 1) {
			hc++;
			if (hc > DHT_MAXCOUNT) {
				printf("Timeout HIGH Pulse %d", i);
				return -1;
			}
		}

		pulseCounts[i] = lc;
		pulseCounts[i+1] = hc;
	}	

	// Normal Scheduler Priority again
	set_default_priority();

	printf("Got all Bits on GPIO %d\n", gpio);

	// Throw away the first two edges (they are always 80µs), and
	// then take the first Low->High edges, which are always the same length.
	// The even numbers are the L->H edges, compute the average cycle time on those.
	int pulseSum = 0;
	for (int i = 2; i < (DHT_PULSES * 2); i = i + 2) {
		pulseSum += pulseCounts[i];
		printf("Pulse %2d, L %d - H %d\n", i, pulseCounts[i], pulseCounts[i+1]);
	}
	int pulseWidth = pulseSum / (DHT_PULSES - 1);

	printf("Average length of pulse in cycles: %d\n", pulseWidth);

	uint8_t data[5];

	// On the other pulse (odd), a length > average cycle is a 1, otherwise 0.
	// Read the 40 pulses and shift the bits
	// (0 for short pulse, 1 for long pulse) into each 5 bytes from the right.
	for (int i = 2; i < (DHT_PULSES * 2); i = i + 2) {
		int idx = (i - 2) / (2 * 8);
		data[idx] = data[idx] << 1;
		if (pulseCounts[i + 1] >= pulseWidth) {
			data[idx] = data[idx] | 1;
		}
	}

	printf("GPIO Bytes = 0x%x 0x%x 0x%x 0x%x 0x%x", 
			data[0], data[1], data[2], data[3], data[4]);

	// Here is a checksum test.
	if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		printf("Checksum mismatch!\n");
		return -1;
	}

	*humidity = (float)data[0];
	*temperature = (float)data[2];

	return 0;
}

int setup(int gpio) {
	int r = 0;
	printf("Searching DHT11 on GPIO %d\n", gpio);

	// Scheduler Re-Nice to -20
	set_max_priority();

	r = gpioSetMode(gpio, PI_INPUT);
	if (r == PI_BAD_MODE) {
		return 1;
	}
	r = gpioSetPullUpDown(gpio, PI_PUD_UP);
	if (r == PI_BAD_MODE) {
		return 1;
	}

	// Let the interface linger around a bit.
	ms_sleep(1000);

	// Set GPIO to OUT and pull it up to HIGH
	r = gpioSetMode(gpio, PI_OUTPUT);
	if (r == PI_BAD_MODE) {
		return 1;
	}

	r = gpioWrite(gpio, 1);
	if (r == PI_BAD_MODE) {
		return 1;
	}

	ms_sleep(500); // 500ms Sleep

	// After a little wait, pull the GPIO down to LOW
	r = gpioWrite(gpio, 0);
	if (r == PI_BAD_MODE) {
		return 1;
	}

	// The protocol in the DHT11-Documentation expects at least 18ms of LOW
	ms_sleep(18); // 18ms Sleep

	// Set GPIO to IN, and wait for the DHT11 to pull it down to LOW
	r = gpioSetMode(gpio, PI_INPUT);
	if (r == PI_BAD_MODE) {
		return 1;
	}

	int t = 0;
	while (gpioRead(gpio) == 1) {
		usleep(1);
		// An arbitrary timeout loop.
		if (t > 32000) {
			return -1;
		}
		t++;
	}

	// Just a hint of delay... To give the DHT11 some time to actually start with the cycle.
	usleep(10);

	return 0;
}
	

/**
 * The following 3 functions are copied from the Adafruit Libary. I have included the License here.
 */

// Copyright (c) 2014 Adafruit Industries
// // Author: Tony DiCola
//
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
//
// // The above copyright notice and this permission notice shall be included in all
// // copies or substantial portions of the Software.
//
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.

void ms_sleep(uint32_t millis) {
	struct timespec sleep;
	sleep.tv_sec = millis / 1000;
	sleep.tv_nsec = (millis % 1000) * 1000000L;
	while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR);
}

void set_max_priority(void) {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance 
	// of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void set_default_priority(void) {
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}
