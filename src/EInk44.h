
#if !defined(EINK44_H)
#define EINK44_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/time.h> 

#include "gpio.h"
#include "spi.h"
#include "EInkImage.h"

#define EINK_WIDTH	 400
#define EINK_HEIGHT 300

#define MAX_TIMEOUT 300000 //200000
#define MAX_DATAPACKET_TIMEOUT 5000
#define MAX_RESPONSE_TIMEOUT 5000
#define MIN_UPDATE_TIMEOUT 100000
#define MAX_UPDATE_TIMEOUT 1500000 //5000000

#define DEFAULT_PACKET_LENGTH 40

#define EN_1 GPIO::GPIO_P9_16
#define CS_1 GPIO::GPIO_P9_15
#define BUSY_1 GPIO::GPIO_P9_25

namespace PDEInkDriver {

class EInk44 {

public:
	EInk44(GPIO::GPIO_pin_type en = EN_1, GPIO::GPIO_pin_type cs = CS_1, GPIO::GPIO_pin_type busy = BUSY_1);
	~EInk44();

	void erase();
	void update();
	void updateFlashless();
	void updateFlashlessInverted();

	void enable();
	void disable();

	bool isBusy();
	void waitUntilFree();
	
	void sendImage(EInkImage& img);
	void sendImage(XBMImage& img);
	bool sendImage(unsigned char * buff, int length, unsigned char packetLength, int retrynum = 0);
	bool sendImageROI(unsigned char * buff, int x, int y, int w, int h, int retrynum = 0);

	void copyImageROI(int x, int y, int w, int h);
	void copyImageROI(int x, int y, int w, int h, int slot);

	void fill(bool white);
	void fillROI(int x, int y, int w, int h, bool white);

private:
	bool _sendImagePacket(unsigned char * buff, int packetNo, unsigned char packetLength, int retrynum = 0);
	bool _sendImagePacket(unsigned char * buff, int packetNo, unsigned char packetLength, unsigned char init, int retrynum = 0);
	bool _resetDataPointer(int retrynum = 0);
	void _sendUpdate(unsigned char transition);
	void _setImageROI(int x, int y, int w, int h);
	void _copyLastSlot(int slot);
	void _uploadImageFixVal(int slot, bool white);

	void _waitForBusy(int timeout);
	int _readResponse(int tryn = 1);

	unsigned char inout[1024];
	SPI* _spi;
	bool _hasBeenInited;

	GPIO::GPIO_pin_type _en;
	GPIO::GPIO_pin_type _cs;
	GPIO::GPIO_pin_type _busy;

	struct timeval last_update_time; 

};

}

#endif