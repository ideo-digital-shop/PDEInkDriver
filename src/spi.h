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


#if !defined(SPI_H)
#define SPI_H 1

#include "gpio.h"

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

namespace PDEInkDriver {

class SPI {

public:
	SPI(const char* spi_path, uint32_t bps);
	SPI(const char* spi_path, uint32_t bps, GPIO::GPIO_pin_type cs_pin);

	~SPI();

	void on();
	void off();

	void enable();
	void disable();

	void send(const void *buffer, size_t length);
	void read(const void *buffer, void *received, size_t length);

private:
	int fd;
	uint32_t bps;
	GPIO::GPIO_pin_type cs_pin;
	bool cs_enable_high;

	void _SPI(const char* spi_path, uint32_t bps, GPIO::GPIO_pin_type cs_pin);
	void set_spi_mode(uint8_t mode);
};

}
#endif
