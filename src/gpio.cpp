// Copyright 2013-2015 Pervasive Displays, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

// device tree base GPIO
// dts files from:  https://github.com/nomel/beaglebone

// OS:
//   root@beaglebone:~# uname -a; lsb_release -a
//   Linux beaglebone 3.8.13 #1 SMP Thu Sep 12 10:27:06 CEST 2013 armv7l GNU/Linux
//   Distributor ID: Angstrom
//   Description:    Angstrom GNU/Linux v2012.12 (Core edition)
//   Release:        v2012.12
//   Codename:       Core edition



#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <err.h>
#include <map>

#include "gpio.h"

namespace PDEInkDriver {
namespace GPIO {

// cape manager info
static char *slots = NULL;
static char *ocp = NULL;


// GPIO

#define MAKE_PIN(_name, _bank, _pin)   \
	gpio_infos[GPIO_PIN(_bank, _pin)] = new GPIO_INFO(_name "\n");

// GPIO states / direction
#define STATE_rxDisable_pullNone "rxDisable_pullNone"
#define STATE_rxEnable_pullNone  "rxEnable_pullNone"
#define STATE_rxDisable_pullUp   "rxDisable_pullUp"
#define STATE_rxEnable_pullUp    "rxEnable_pullUp"
#define STATE_rxDisable_pullDown "rxDisable_pullDown"
#define STATE_rxEnable_pullDown  "rxEnable_pullDown"

#define DIRECTION_in  "in"
#define DIRECTION_out "out"

// PWM
static struct {
	char *name;
	int fd;
	uint32_t period;
} pwm[4];


// compute array size at compile time
#define SIZE_OF_ARRAY(a) (sizeof(a) / sizeof((a)[0]))

// get the size of a constant string at compile time
// no need for extra strlen at run time
// (note ignore the trailing '\0')
#define CONST_STRLEN(const_string) (sizeof(const_string) - sizeof((char)('\0')))



// local function prototypes;
static bool load_firmware(const char *pin_name);
static void write_file(const char *file_name, const char *buffer, size_t length);
static void export_pin(const char *pin_number);
static void unexport(const char *pin_number);
static bool GPIO_enable(int pin);
static bool PWM_enable(int channel, const char *pin_name);
static void PWM_set_duty(int channel, int16_t value);


// GPIO control information
struct GPIO_INFO {
	const char *name;   // e.g. "gpio-P8.15" -> /lib/firmware/gpio-P8.15.dtbo
			    // alse to search cape manager slots to avoid reload
	char *state;        // e.g. "/sys/devices/ocp.3/gpio-P8.15_gpio47.17/state"
			    // set to any of the STATE_xxx above
	char *number;       // e.g. "47"  from the "_gpioXX." when determining state path
			    // use number with: "/sys/class/gpio/export" (or unexport)
	char *value;        // e.g. "/sys/class/gpio/gpio47/value" <- [ "0" | "1" ]
	char *active_low;   // e.g. "/sys/class/gpio/gpio47/active_low" <- [ "0" | "1" ]
	char *direction;    // e.g. "/sys/class/gpio/gpio47/direction" <- DIRECTION_xxx
	int fd;             // open fd to value file for fast access

	GPIO_INFO():
		name(""),
		state(NULL),
		number(NULL),
		value(NULL),
		active_low(NULL),
		direction(NULL),
		fd(-1){
	}

	GPIO_INFO(const char* _name):
		name(_name),
		state(NULL),
		number(NULL),
		value(NULL),
		active_low(NULL),
		direction(NULL),
		fd(-1){
	}

	~GPIO_INFO(){
		if (fd >= 0) {
			close(fd);
			fd = -1;
		}
		if (NULL != state) {
			free(state);
			state = NULL;
		}
		if (NULL != number) {
			unexport(number);
			free(number);
			number = NULL;
		}
		if (NULL != direction) {
			free(direction);
			direction = NULL;
		}
		if (NULL != active_low) {
			free(active_low);
			active_low = NULL;
		}
		if (NULL != value) {
			free(value);
			value = NULL;
		}
	}
};

typedef std::map<int, GPIO_INFO*> gpio_info_t; 
typedef std::map<int, GPIO_INFO*>::iterator it_gpio_info;
static gpio_info_t gpio_infos;

GPIO_INFO* GPIO_get_info(int pin){
	gpio_info_t::iterator i = gpio_infos.find(pin);
	if(i == gpio_infos.end()){
		gpio_infos[pin] = new GPIO_INFO();
		return gpio_infos[pin];
	}
	return i->second;
}

void load_pins(){
	// Connector P8
	MAKE_PIN("gpio-P8.03", 1, 6)   //  GPIO1_6
	MAKE_PIN("gpio-P8.04", 1, 7)   //  GPIO1_7
	MAKE_PIN("gpio-P8.05", 1, 2)   //  GPIO1_2
	MAKE_PIN("gpio-P8.06", 1, 3)   //  GPIO1_3
	MAKE_PIN("gpio-P8.07", 2, 2)   //  GPIO2_2   TIMER4
	MAKE_PIN("gpio-P8.08", 2, 3)   //  GPIO2_3   TIMER7
	MAKE_PIN("gpio-P8.09", 2, 5)   //  GPIO2_5   TIMER5
	MAKE_PIN("gpio-P8.10", 2, 4)   //  GPIO2_4   TIMER6
	MAKE_PIN("gpio-P8.11", 1, 13)  //  GPIO1_13
	MAKE_PIN("gpio-P8.12", 1, 12)  //  GPIO1_12
	MAKE_PIN("gpio-P8.13", 0, 23)  //  GPIO0_23  EHRPWM2B
	MAKE_PIN("gpio-P8.14", 0, 26)  //  GPIO0_26
	MAKE_PIN("gpio-P8.15", 1, 15)  //  GPIO1_15
	MAKE_PIN("gpio-P8.16", 1, 14)  //  GPIO1_14
	MAKE_PIN("gpio-P8.17", 0, 27)  //  GPIO0_27
	MAKE_PIN("gpio-P8.18", 2, 1)   //  GPIO2_1
	MAKE_PIN("gpio-P8.19", 0, 22)  //  GPIO0_22  EHRPWM2A
	MAKE_PIN("gpio-P8.20", 1, 31)  //  GPIO1_31
	MAKE_PIN("gpio-P8.21", 1, 30)  //  GPIO1_30
	MAKE_PIN("gpio-P8.22", 1, 5)   //  GPIO1_5
	MAKE_PIN("gpio-P8.23", 1, 4)   //  GPIO1_4
	MAKE_PIN("gpio-P8.24", 1, 1)   //  GPIO1_1
	MAKE_PIN("gpio-P8.25", 1, 0)   //  GPIO1_0
	MAKE_PIN("gpio-P8.26", 1, 29)  //  GPIO1_29
	MAKE_PIN("gpio-P8.27", 2, 22)  //  GPIO2_22
	MAKE_PIN("gpio-P8.28", 2, 24)  //  GPIO2_24
	MAKE_PIN("gpio-P8.29", 2, 23)  //  GPIO2_23
	MAKE_PIN("gpio-P8.30", 2, 25)  //  GPIO2_25
	MAKE_PIN("gpio-P8.31", 0, 10)  //  GPIO0_10  UART5_CTSN
	MAKE_PIN("gpio-P8.32", 0, 11)  //  GPIO0_11  UART5_RTSN
	MAKE_PIN("gpio-P8.33", 0, 9)   //  GPIO0_9   UART4_RTSN
	MAKE_PIN("gpio-P8.34", 2, 17)  //  GPIO2_17  UART3_RTSN
	MAKE_PIN("gpio-P8.35", 0, 8)   //  GPIO0_8   UART4_CTSN
	MAKE_PIN("gpio-P8.36", 2, 16)  //  GPIO2_16  UART3_CTSN
	MAKE_PIN("gpio-P8.37", 2, 14)  //  GPIO2_14  UART5_TXD
	MAKE_PIN("gpio-P8.38", 2, 15)  //  GPIO2_15  UART5_RXD
	MAKE_PIN("gpio-P8.39", 2, 12)  //  GPIO2_12
	MAKE_PIN("gpio-P8.40", 2, 13)  //  GPIO2_13
	MAKE_PIN("gpio-P8.41", 2, 10)  //  GPIO2_10
	MAKE_PIN("gpio-P8.42", 2, 11)  //  GPIO2_11
	MAKE_PIN("gpio-P8.43", 2, 8)   //  GPIO2_8
	MAKE_PIN("gpio-P8.44", 2, 9)   //  GPIO2_9
	MAKE_PIN("gpio-P8.45", 2, 6)   //  GPIO2_6
	MAKE_PIN("gpio-P8.46", 2, 7)   //  GPIO2_7

	// Connector P9
	MAKE_PIN("gpio-P9.11", 0, 30)  //  GPIO0_30  UART4_RXD
	MAKE_PIN("gpio-P9.12", 1, 28)  //  GPIO1_28
	MAKE_PIN("gpio-P9.13", 0, 31)  //  GPIO0_31  UART4_TXD
	MAKE_PIN("gpio-P9.14", 1, 18)  //  GPIO1_18  EHRPWM1A
	MAKE_PIN("gpio-P9.15", 1, 16)  //  GPIO1_16
	MAKE_PIN("gpio-P9.16", 1, 19)  //  GPIO1_19  EHRPWM1B
	MAKE_PIN("gpio-P9.17", 0, 5)   //  GPIO0_5   I2C1_SCL
	MAKE_PIN("gpio-P9.18", 0, 4)   //  GPIO0_4   I2C1_SDA
	MAKE_PIN("gpio-P9.19", 0, 13)  //  GPIO0_13  I2C2_SCL
	MAKE_PIN("gpio-P9.20", 0, 12)  //  GPIO0_12  I2C2_SDA
	MAKE_PIN("gpio-P9.21", 0, 3)   //  GPIO0_3   UART2_TXD
	MAKE_PIN("gpio-P9.22", 0, 2)   //  GPIO0_2   UART2_RXD
	MAKE_PIN("gpio-P9.23", 1, 17)  //  GPIO1_17
	MAKE_PIN("gpio-P9.24", 0, 15)  //  GPIO0_15  UART1_TXD
	MAKE_PIN("gpio-P9.25", 3, 21)  //  GPIO3_21
	MAKE_PIN("gpio-P9.26", 0, 14)  //  GPIO0_14  UART1_RXD
	MAKE_PIN("gpio-P9.27", 3, 19)  //  GPIO3_19
	MAKE_PIN("gpio-P9.28", 3, 17)  //  GPIO3_17  SPI1_CS0
	MAKE_PIN("gpio-P9.29", 3, 15)  //  GPIO3_15  SPI1_D0
	MAKE_PIN("gpio-P9.30", 3, 16)  //  GPIO3_16  SPI1_D1
	MAKE_PIN("gpio-P9.31", 3, 14)  //  GPIO3_14  SPI1_SCLK
}

// set up access to the GPIO and PWM
bool GPIO_setup() {

	// Set up pin storage
	load_pins();

	// ensure the base firmware is setup
	if (load_firmware(NULL)) {
		// return success
		return true;
	}

	// shudown anythin that was created
	GPIO_teardown();
	return false;
}


/// revoke access to GPIO and PWM
bool GPIO_teardown() {
	size_t i;
	while (!gpio_infos.empty())
	{
		GPIO_INFO* info = gpio_infos.begin()->second;
		free(info);
		gpio_infos.erase(gpio_infos.begin());
	}

	for (i = 0; i < SIZE_OF_ARRAY(pwm); ++i) {
		if (NULL != pwm[i].name) {
			close(pwm[i].fd);
			pwm[i].fd = -1;
			free(pwm[i].name);
			pwm[i].name = NULL;
		}
	}

	if (NULL != slots) {
		// should unload the loaded firmware here,
		// but any attempt to do this crashes the process
		// probably a kernel bug, which may cause system instablility
		free(slots);
		slots = NULL;
	}

	if (NULL != ocp) {
		free(ocp);
		ocp = NULL;
	}

	// all data cleared so calling setup again will work

	return true;
}


void GPIO_mode(int pin, GPIO_mode_type mode) {

	// ignore unimplemented pins
	if (pin < 0){ // || pin >= SIZE_OF_ARRAY(gpio_info)) {
		return;
	}

	GPIO_INFO* _gpio = GPIO_get_info(pin);

	switch (mode) {
	default:
	case GPIO_INPUT:
		if (_gpio->fd < 0 && !GPIO_enable(pin)) {
			return;
		}
		write_file(_gpio->direction, DIRECTION_in "\n", CONST_STRLEN(DIRECTION_in "\n"));
		write_file(_gpio->active_low, "0\n", 2);
		write_file(_gpio->state, STATE_rxEnable_pullNone "\n", CONST_STRLEN(STATE_rxEnable_pullNone "\n"));
		break;

	case GPIO_OUTPUT:
		if (_gpio->fd < 0 && !GPIO_enable(pin)) {
			return;
		}
		write_file(_gpio->direction, DIRECTION_out "\n", CONST_STRLEN(DIRECTION_out "\n"));
		write_file(_gpio->active_low, "0\n", 2);
		write_file(_gpio->state, STATE_rxDisable_pullNone "\n", CONST_STRLEN(STATE_rxDisable_pullNone "\n"));
		break;

	case GPIO_PWM:  // only certain pins allowed
		switch (pin) {
		case GPIO_P9_14:    // EHRPWM1A
			PWM_enable(0, "P9_14");
			break;
		case GPIO_P9_16:    // EHRPWM1B
			PWM_enable(1, "P9_16");
			break;
		case GPIO_P8_19:    // EHRPWM2A
			PWM_enable(2, "P8_19");
			break;
		case GPIO_P8_13:    // EHRPWM2B
			PWM_enable(3, "P8_13");
			break;
		default:
			break;
		}
		break;
	}
}


int GPIO_read(int pin) {
	// ignore unimplemented or inactive pins
	if (pin < 0) { // {
		return 0;
	}

	GPIO_INFO* _gpio = GPIO_get_info(pin);
	if(NULL == _gpio || NULL == _gpio->name || _gpio->fd < 0){
		return 0;
	}

	int fd = _gpio->fd;
	char buffer[2];
	buffer[0] = 0;

	lseek(fd, 0, SEEK_SET);
	read(fd, buffer, sizeof(buffer));

	if ('1' == buffer[0]) {
		return 1;
	} else {
		return 0;
	}
}


void GPIO_write(int pin, int value) {
	// ignore unimplemented or inactive pins
	if (pin < 0){
		return;
	}

	GPIO_INFO* _gpio = GPIO_get_info(pin);
	if(NULL == _gpio || NULL == _gpio->name || _gpio->fd < 0){
		return;
	}

	int fd = _gpio->fd;

	lseek(fd, 0, SEEK_SET);

	// printf("[GPIO] %d : %d\n", pin, value);

	if (0 == value) {
		write(fd, "0\n", 2);
	} else {
		write(fd, "1\n", 2);
	}
	fsync(fd);
}


// only affetct PWM if correct pin is addressed
void GPIO_pwm_write(int pin, uint32_t value) {
	if (value > 1023) {
		value = 1023;
	}
	switch (pin) {
	case GPIO_P9_14:    // EHRPWM1A
		PWM_set_duty(0, value);
		break;
	case GPIO_P9_16:    // EHRPWM1B
		PWM_set_duty(1, value);
		break;
	case GPIO_P8_19:    // EHRPWM2A
		PWM_set_duty(2, value);
		break;
	case GPIO_P8_13:    // EHRPWM2B
		PWM_set_duty(3, value);
		break;
	default:
		break;
	}
}


// private functions
// =================

#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#define SYS_EXPORT   "/sys/class/gpio/export"
#define SYS_UNEXPORT "/sys/class/gpio/unexport"

#define SYS_DEVICES "/sys/devices"
#define CAPE_MANAGER "bone_capemgr."
#define OCP "ocp."

#define SLOTS "/slots"

// GPIO files
#define SYS_CLASS_GPIO "/sys/class/gpio/gpio"
#define STATE "state"
#define DIRECTION "direction"
#define ACTIVE_LOW "active_low"
#define VALUE "value"


// pwm files
#define PERIOD "/period"
#define DUTY "/duty"
#define RUN "/run"
#define POLARITY "/polarity"

// firmware files (/lib/firmware/NAME.dtbo) to load
#define CAPE_IIO "cape-bone-iio"
#define CAPE_PWM "am33xx_pwm"
#define CAPE_SPI "BB-SPIDEV1"
#define CAPE_PWM_PIN_PREFIX "bone_pwm_"

// ocp files
#define OCP_PWM_PREFIX "pwm_test_"


static bool find_slots_and_ocp() {
	if (NULL != slots) {
		return true;  // already done
	}

	DIR *dir = opendir(SYS_DEVICES);
	if (NULL == dir) {
		return false; // failed
	}

	struct dirent *dp;
	while (NULL != (dp = readdir(dir))) {
		if (0 == strncmp(dp->d_name, CAPE_MANAGER, CONST_STRLEN(CAPE_MANAGER))
		    && NULL == slots) {
			slots = (char*)malloc(CONST_STRLEN(SYS_DEVICES) + sizeof((char)('/')) + strlen(dp->d_name)
				       + CONST_STRLEN(SLOTS) + sizeof((char)('\0')));
			if (NULL == slots) {
				break;   // failed
			}
			strcpy(slots, SYS_DEVICES "/");
			strcat(slots, dp->d_name);
			strcat(slots, SLOTS);
		} else if (0 == strncmp(dp->d_name, OCP, CONST_STRLEN(OCP))
			   && NULL == ocp) {
			ocp = (char*)malloc(CONST_STRLEN(SYS_DEVICES)
				     + sizeof((char)('/'))
				     + strlen(dp->d_name)
				     + sizeof((char)('\0')));
			if (NULL == ocp) {
				break;   // failed
			}
			strcpy(ocp, SYS_DEVICES "/");
			strcat(ocp, dp->d_name);
		}
	}
	(void)closedir(dir);

	if (NULL == slots || NULL == ocp) {
		if (NULL != slots) {
			free(slots);
			slots = NULL;
		}
		if (NULL != ocp) {
			free(ocp);
			ocp = NULL;
		}

		return false;  // failed
	}

	// success
	return true;
}


static bool load_firmware(const char *pin_name) {

	// find slots and ocp paths
	if (!find_slots_and_ocp()) {
		return false;  // failed
	}

	int fd = open(slots, O_RDWR);
	if (fd < 0) {
		return false;  // failed
	}

	char buffer[8192];  // only 4096 is indicated in sysfs, can it be larger?

	memset(buffer, 0, sizeof(buffer));
	read(fd, buffer, sizeof(buffer) - 1);  // allow one nul at end

	// IO multiplexing
	if (NULL == strstr(buffer, CAPE_IIO)) {
		lseek(fd, 0, SEEK_SET);
		write(fd, CAPE_IIO "\n", CONST_STRLEN(CAPE_IIO "\n"));
	}
	// PWM
	if (NULL == strstr(buffer, CAPE_PWM)) {
		lseek(fd, 0, SEEK_SET);
		write(fd, CAPE_PWM "\n", CONST_STRLEN(CAPE_PWM "\n"));
	}
	// also set up SPI0 -> /dev/spidevX.Y
	if (NULL == strstr(buffer, CAPE_SPI)) {
		lseek(fd, 0, SEEK_SET);
		write(fd, CAPE_SPI "\n", CONST_STRLEN(CAPE_SPI "\n"));
	}

	// The desired I/O pin
	if (NULL != pin_name) {
		lseek(fd, 0, SEEK_SET);
		write(fd, pin_name, strlen(pin_name));
	}
	// finished with the cape manager
	close(fd);
	return true;
}

static void write_file(const char *file_name, const char *buffer, size_t length) {
	if (length <= 0) {
		length = strlen(buffer);
	}
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "write_file failed: '%s' <- '%s'\n", file_name, buffer); fflush(stderr);
		return;  // failed
	}
	size_t n = write(fd, buffer, length);
	fsync(fd);
	if (n != length) {
		// fprintf(stderr, "write_file only wrote: %d of %d\n", n, length); fflush(stderr);
	}
	close(fd);
}


static void export_pin(const char *pin_number) {
	write_file(SYS_EXPORT, pin_number, 0);
}


static void unexport(const char *pin_number) {
	write_file(SYS_UNEXPORT, pin_number, 0);
}


// enable GPIO
static bool GPIO_enable(int pin) {
	if (pin < 0){ // || pin >= SIZE_OF_ARRAY(gpio_info) || NULL == gpio_info[pin].name) {
		return false; // invalid pin
	}

	GPIO_INFO* _gpio = GPIO_get_info(pin);

	if (NULL != _gpio->state) {
		return true;  // already configured
	}

	// try to load its firmware
	if (!load_firmware(_gpio->name)) {
		return false;  // failed
	}

	// wait a bit for the device to appear
	// is this long enough or should the whole code below be in a retry loop
	usleep(10000);

	size_t length = strlen(_gpio->name) - 1;  // ignore trailing '\n'

	// find pwm path
	DIR *dir = opendir(ocp);
	if (NULL == dir) {
		return false;  // failed
	}
	struct dirent *dp;
	while (NULL != (dp = readdir(dir))) {
		if (0 == strncmp(dp->d_name, _gpio->name, length)) {

			_gpio->state = (char*)malloc(strlen(ocp)
						      + sizeof((char)('/'))
						      + strlen(dp->d_name)
						      + sizeof((char)('/'))
						      + CONST_STRLEN(STATE)
						      + sizeof((char)('\0')));
			if (NULL == _gpio->state) {
				break;  // failed
			}

			// state - for setting optianl pullup/pulldown
			strcpy(_gpio->state, ocp);
			strcat(_gpio->state, "/");
			strcat(_gpio->state, dp->d_name);
			strcat(_gpio->state, "/");
			strcat(_gpio->state, STATE);

			const char *p = &(dp->d_name[length]);
			while ('\0' != *p && !isdigit(*p)) {
				++p;
			}
			size_t l = strspn(p, "0123456789");
			if (l <= 0) {
				break;  // failed
			}

			// save the GPIO number - the 'pin' variable probably has
			// the same value, but it is safer to use the kernel provided value
			// in case the kernel logic changes
			_gpio->number = (char*)malloc(l +  sizeof((char)('/')) + sizeof((char)('\0')));
			if (NULL == _gpio->number) {
				break;  // failed
			}
			strncpy(_gpio->number, p, l);
			_gpio->number[l] = '\n';
			_gpio->number[l + 1] = '\0';

			// as the kernel to allocate the pin
			export_pin(_gpio->number);

			// the direction file name
			_gpio->direction = (char*)malloc(CONST_STRLEN(SYS_CLASS_GPIO)
							  + l
							  + sizeof((char)('/'))
							  + CONST_STRLEN(DIRECTION)
							  + sizeof((char)('\0')));
			if (NULL == _gpio->direction) {
				break;  // failed
			}

			strcpy(_gpio->direction, SYS_CLASS_GPIO);
			strncat(_gpio->direction, p, l);
			strcat(_gpio->direction, "/");
			strcat(_gpio->direction, DIRECTION);

			// the active low file name
			_gpio->active_low = (char*)malloc(CONST_STRLEN(SYS_CLASS_GPIO)
							  + l
							  + sizeof((char)('/'))
							  + CONST_STRLEN(ACTIVE_LOW)
							  + sizeof((char)('\0')));
			if (NULL == _gpio->active_low) {
				break;  // failed
			}

			strcpy(_gpio->active_low, SYS_CLASS_GPIO);
			strncat(_gpio->active_low, p, l);
			strcat(_gpio->active_low, "/");
			strcat(_gpio->active_low, ACTIVE_LOW);

			// the value file name
			_gpio->value = (char*)malloc(CONST_STRLEN(SYS_CLASS_GPIO)
						      + l
						      + sizeof((char)('/'))
						      + CONST_STRLEN(VALUE)
						      + sizeof((char)('\0')));
			if (NULL == _gpio->value) {
				break;  // failed
			}

			strcpy(_gpio->value, SYS_CLASS_GPIO);
			strncat(_gpio->value, p, l);
			strcat(_gpio->value, "/");
			strcat(_gpio->value, VALUE);

			// open a file handle to the value - to speed
			// up access assumes most read/write go to
			// this as other items (like direction) are
			// only changed oocaisionally.
			_gpio->fd = open(_gpio->value, O_RDWR | O_EXCL);
			if (_gpio->fd < 0) {
				break;  // failed
			}

			return true;

			break;
		}
	}
	closedir(dir);

	// clean up any allocated memory or descriptors
	if (_gpio->fd >= 0) {
		close(_gpio->fd);
		_gpio->fd = -1;
	}
	if (NULL != _gpio->state) {
		free(_gpio->state);
		_gpio->state = NULL;
	}
	if (NULL != _gpio->number) {
		unexport(_gpio->number);
		free(_gpio->number);
		_gpio->number = NULL;
	}
	if (NULL != _gpio->direction) {
		free(_gpio->direction);
		_gpio->direction = NULL;
	}
	if (NULL != _gpio->active_low) {
		free(_gpio->active_low);
		_gpio->active_low = NULL;
	}
	if (NULL != _gpio->value) {
		free(_gpio->value);
		_gpio->value = NULL;
	}

	return false; // failed
}


// enable PWM
static bool PWM_enable(int channel, const char *pin_name) {
	if (NULL != pwm[channel].name) {
		return true;  // already configured
	}

	// compose PWM pin name
	char *config = (char*)malloc(CONST_STRLEN(CAPE_PWM_PIN_PREFIX)
			      + strlen(pin_name)
			      + sizeof((char)('\n'))
			      + sizeof((char)('\0')));

	if (NULL == config) {
		return false;  // failed
	}
	strcpy(config, CAPE_PWM_PIN_PREFIX);
	strcat(config, pin_name);
	strcat(config, "\n");

	// try to load its firmware
	if (!load_firmware(config)) {
		free(config);
		return false;  // failed
	}
	free(config);

	// wait a bit for the pwm device to appear
	// is this long enough or should the whole code below be in a retry loop
	usleep(10000);

	config = (char*)malloc(CONST_STRLEN(OCP_PWM_PREFIX)
			      + strlen(pin_name)
			      + sizeof((char)('\0')));

	if (NULL == config) {
		return false; // failed
	}

	strcpy(config, OCP_PWM_PREFIX);
	strcat(config, pin_name);
	size_t length = strlen(config);

	// find pwm path
	DIR *dir = opendir(ocp);
	if (NULL == dir) {
		goto done; // failed
	}
	struct dirent *dp;
	while (NULL != (dp = readdir(dir))) {
		if (0 == strncmp(dp->d_name, config, length)) {
			pwm[channel].name = (char*)malloc(strlen(ocp)
						   + sizeof((char)('/'))
						   + strlen(dp->d_name) + CONST_STRLEN(DUTY)
						   + sizeof((char)('\0')));
			if (NULL == pwm[channel].name) {
				break;  // failed
			}

			strcpy(pwm[channel].name, ocp);
			strcat(pwm[channel].name, "/");
			strcat(pwm[channel].name, dp->d_name);
			strcat(pwm[channel].name, DUTY);

			// wait up to 5 seconds for the pwm driver to appear
			// is the device tree populated in the background?
			int i;
			for (i = 0; i < 500; ++i) {
				pwm[channel].fd = open(pwm[channel].name, O_RDWR);
				if (pwm[channel].fd >= 0) {
					break;
				}
				usleep(10000);
			}
			if (pwm[channel].fd < 0) {
				fprintf(stderr, "PWM failed to appear\n"); fflush(stderr);
				free(pwm[channel].name);
				pwm[channel].name = NULL;
				break;  // failed
			}

			// set duty = zero
			lseek(pwm[channel].fd, 0, SEEK_SET);
			write(pwm[channel].fd, "0\n", 2);

			char buffer[4096];
			// set zero duty => zero output
			strcpy(buffer, ocp);
			strcat(buffer, "/");
			strcat(buffer, dp->d_name);
			strcat(buffer, POLARITY);

			write_file(buffer, "0\n", 2);

			// read and save period?
			// ???currently seems to be fixed to 500000
			pwm[channel].period = 500000;

			// start
			strcpy(buffer, ocp);
			strcat(buffer, "/");
			strcat(buffer, dp->d_name);
			strcat(buffer, RUN);

			write_file(buffer, "1\n", 2);

			break;
		}
	}
	closedir(dir);

done:
	free(config);
	return NULL != pwm[channel].name;
}


static void PWM_set_duty(int channel, int16_t value) {
	if (channel < 0 || channel > 3) {
		return;
	}
	if (value < 0) {
		value = 0;
	} else if (value > 1023) {
		value = 1023;
	}

	uint32_t duty = value * pwm[channel].period / 1023;

	char config[256];
	snprintf(config, sizeof(config), "%d\n", duty),

	lseek(pwm[channel].fd, 0, SEEK_SET);
	write(pwm[channel].fd, config, strlen(config));
	fsync(pwm[channel].fd);
}

}

}
