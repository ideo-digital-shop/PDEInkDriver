

#include "EInk44.h"

#define DEBUG false
namespace PDEInkDriver {

EInk44::EInk44(GPIO::GPIO_pin_type en, GPIO::GPIO_pin_type cs, GPIO::GPIO_pin_type busy){

	_en = en;
	_cs = cs;
	_busy = busy;

	_spi = new SPI("/dev/spidev1.0", 8000000, _cs);
	_hasBeenInited = false;
	if (NULL == _spi) {
		warn("SPI_setup failed");
	} else {


		GPIO::GPIO_mode(_busy, GPIO::GPIO_INPUT);
		GPIO::GPIO_mode(_en, GPIO::GPIO_OUTPUT);
		GPIO::GPIO_mode(_cs, GPIO::GPIO_OUTPUT);
		usleep(5 * 000);

		GPIO_write(_en, 1);
		GPIO_write(_cs, 0);

		usleep(5 * 1000);

		GPIO_write(_en, 0);
		GPIO_write(_cs, 0);

		usleep(25 * 1000);

		GPIO_write(_en, 0);
		GPIO_write(_cs, 1);

		_spi->on();
	}
}

EInk44::~EInk44(){
	if(_spi){
		_spi->off();
		delete _spi;
	}
}

void EInk44::enable(){
	if(DEBUG) printf("[EINK] Enable\n");
	GPIO::GPIO_write(_en, 0);
}

void EInk44::disable(){
	if(DEBUG) printf("[EINK] Disable\n");
	GPIO::GPIO_write(_en, 1);
}

// Erase the EInk Screen
void EInk44::erase(){
	if(DEBUG) printf("Erase display. ");
	inout[0] = 0x20;
	inout[1] = 0x0E;
	inout[2] = 0x00; //slot;
	inout[3] = 0x00;
	inout[4] = 0x00;
	inout[5] = 0x00;

	_spi->enable();
	_spi->send(inout, 3);
	_spi->disable();
	if(DEBUG) printf("\n");

	_waitForBusy(MAX_TIMEOUT);
}

void EInk44::update(){
	_sendUpdate(0x24);
}

void EInk44::updateFlashless(){
	_sendUpdate(0x85);
}

void EInk44::updateFlashlessInverted(){
	_sendUpdate(0x86);
}

void EInk44::sendImage(EInkImage& img){
	sendImage(img.bits(), img.length(), DEFAULT_PACKET_LENGTH);
}

void EInk44::sendImage(XBMImage& img){
	sendImage(img.bits(), img.length(), DEFAULT_PACKET_LENGTH);	
}

bool EInk44::sendImage(unsigned char * buff, int length, unsigned char packetLength, int retrynum){
	int i;
	int packets = (length / packetLength) + 1;
	int lastPacketLength = length % packetLength;
	if(DEBUG) printf("Send image(%d, %d, %d, %d).\n", length, packetLength, packets, lastPacketLength);
	if(DEBUG) printf("=================================\n");

	_resetDataPointer();

	bool imageWrite = true;
	for(i = 0; i < packets - 1; i++)
	  imageWrite &= _sendImagePacket(&buff[i*packetLength], i, packetLength);
	imageWrite &= _sendImagePacket(&buff[i*packetLength], 0, lastPacketLength);
	if(DEBUG) printf("\n");

	return imageWrite;
}


bool EInk44::sendImageROI(unsigned char * buff, int x, int y, int w, int h, int retrynum){

	// Make sure it doesnt go out of bounds
	h = (y + h > 400) ? 400 - y : h;
	int packetLength = (w > 250) ? w / 2 : w;
	int i;
	int length = w * h / 8;
	int packets = (length / packetLength) + 1;
	int lastPacketLength = length % packetLength;

	_setImageROI(x, y, w, h);

	if(DEBUG) printf("Send image(%d, %d, %d, %d).\n", length, packetLength, packets, lastPacketLength);
	if(DEBUG) printf("=================================\n");
	bool imageWrite = false;
	for(i = 0; i < packets - 1; i++)
		imageWrite |= _sendImagePacket(&buff[i*packetLength], i, packetLength);
	if(lastPacketLength > 0){
		imageWrite |= _sendImagePacket(&buff[i*packetLength], 0, lastPacketLength);
	}

	if(!imageWrite && retrynum == 0){
		sendImageROI(buff, x, y, w, h, retrynum + 1);
	}

	return imageWrite;
}

void EInk44::fill(bool white){
	_setImageROI(0, 0, EINK_WIDTH, EINK_HEIGHT);
	usleep(1000);
	_uploadImageFixVal(0, white);
}

void EInk44::fillROI(int x, int y, int w, int h, bool white){
	_setImageROI(x, y, w, h);
	usleep(1000);
	_uploadImageFixVal(0, white);
}

void EInk44::copyImageROI(int x, int y, int w, int h, int slot){
	_setImageROI(x, y, w, h);
	usleep(10000);
	_copyLastSlot(slot);
}

void EInk44::copyImageROI(int x, int y, int w, int h){
	copyImageROI(x, y, w, h, -1);
}


bool EInk44::isBusy(){
	struct timeval end_time; 
	gettimeofday( &end_time, NULL ); 
    int elapsed_time = (end_time.tv_sec - last_update_time.tv_sec) * 1e6 + 
    	( end_time.tv_usec - last_update_time.tv_usec ) ;
    if(elapsed_time < MIN_UPDATE_TIMEOUT){
    	return true;
    }

	if(GPIO::GPIO_read(_busy) == 0){
		if(elapsed_time < MAX_UPDATE_TIMEOUT){
	    	return true;
	    }
    }
    return false;
}

/* Private Helpers */

void EInk44::_sendUpdate(unsigned char transition){
	if(DEBUG) printf("Display update...");
	inout[0] = transition;
	inout[1] = 0x01;
	inout[2] = 0x00; //slot;
	inout[3] = 0x00;
	inout[4] = 0x00;
	inout[5] = 0x00;

	_spi->enable();
	_spi->send(inout, 3);
	_spi->disable();
	if(DEBUG) printf("\n");

	gettimeofday( &last_update_time, NULL ); 

	// _waitForBusy(MAX_UPDATE_TIMEOUT);
}

int EInk44::_readResponse(int tryn){
	// return true;
	// Read response

	if(tryn > 2){
		return false;
	}

	inout[0] = 0x00;
	inout[1] = 0x00;
	inout[2] = 0x00;
	inout[3] = 0x00;
	inout[4] = 0x00;
	inout[5] = 0x00;
	
	_spi->enable();
	_spi->read(inout, inout, 2);
	_spi->disable();

	_waitForBusy(MAX_RESPONSE_TIMEOUT);
	
	if((inout[0] == 0x00 && inout[1] == 0x00) || (inout[0] == 0xFF && inout[1] == 0xFF)){
		if(DEBUG) printf("[EINK] [Unable to get proper response] [%d]: 0x%x 0x%x\n", tryn, inout[0], inout[1]);
		return _readResponse(tryn + 1);
	}

	if(inout[0] != 0x90 || inout[1] != 0x00){
		usleep(1000);
		if(DEBUG) printf("[EINK] Response: 0x%x 0x%x\n", inout[0], inout[1]);
		if(inout[0] == 0x67 && inout[1] == 0x00){
			return 0x6700;
		} else if(inout[0] == 0x6A && inout[1] == 0x00){
			return 0x6A00;
		} else {
			return 0xF0F0;
		}
	}
	// printf("[EINK] [GOOD]: 0x%x 0x%x\n", inout[0], inout[1]);
	
	return 0x9000;
}

bool EInk44::_sendImagePacket(unsigned char * buff, int packetNo, unsigned char packetLength, int retrynum)
{
	if(_hasBeenInited){
		return _sendImagePacket(buff, packetNo, packetLength, 0x01, 0);
	} else {
		_hasBeenInited = true;
		return _sendImagePacket(buff, packetNo, packetLength, 0x01, 0); //0x49, 0);
	}
}

bool EInk44::_sendImagePacket(unsigned char * buff, int packetNo, unsigned char packetLength, unsigned char init, int retrynum){
	if(retrynum > 5){
		return false;
	}
	// printf("Send image packet(%d, %d). ", packetNo, packetLength);
	inout[0] = 0x20;
	inout[1] = init;
	inout[2] = 0x00; // slot;
	inout[3] = packetLength;
	memcpy(&inout[4], buff, packetLength);

	_spi->enable();
	_spi->send(inout, 4 + packetLength);
	_spi->disable();

	// Wait till its free
	_waitForBusy(MAX_DATAPACKET_TIMEOUT);

	int response = _readResponse();
	if(response != 0x9000){
		if(DEBUG) printf("[EINK] [ERROR] Invalid send image packet: _sendImagePacket(%d, %d, 0x%x)\n", packetNo, packetLength, init);
		if(true || response == 0x6D00 || response == 0x6700){
			printf("Send Failed.\n");
			if(retrynum < 3){
				return _sendImagePacket(buff, packetNo, packetLength, init, retrynum + 1);
			} else {
				return false;
			}
		}
	}

	return true;
}

bool EInk44::_resetDataPointer(int retrynum){
	if(DEBUG) printf("Reset data pointer. ");

	if(retrynum > 2){
		return false;
	}

	// disable();
	// usleep(1000);
	// enable();

	// return;

	inout[0] = 0x20;
	inout[1] = 0x0D;
	inout[2] = 0x00;
	inout[3] = 0x00;
	inout[4] = 0x00;
	inout[5] = 0x00;

	_spi->enable();
	_spi->send(inout, 3);
	_spi->disable();

	if(DEBUG) printf("\n"); 

	_waitForBusy(MAX_TIMEOUT);

	if(_readResponse() == 0x6700){
		if(DEBUG) printf("[EINK] [ERROR] Invalid reset data pointer. Try again...\n");
		return _resetDataPointer(retrynum + 1);
	}
	return true;
}

void EInk44::waitUntilFree(){
	_waitForBusy(MAX_TIMEOUT);
	while(isBusy()) {
		usleep(10);
	}
}

void EInk44::_waitForBusy(int timeout){
	usleep(1000);
	int elapsed_time; 
    struct timeval start_time, end_time; 
    gettimeofday( &start_time, NULL ); 
    // printf("Elapsed time: 0\n");
    while(GPIO::GPIO_read(_busy) == 0){
		gettimeofday( &end_time, NULL ); 
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1e6 + 
        	( end_time.tv_usec - start_time.tv_usec ) ;
        // printf("Elapsed time: %d / %d\n", elapsed_time, timeout);
		if(elapsed_time > timeout){
			printf("[TIMEOUT!!] %d\n", elapsed_time);
			break;
		}
		usleep(100);
	}
}

void EInk44::_copyLastSlot(int slot){
	inout[0] = 0x20;
	inout[1] = 0x0C;
	inout[2] = 0x00;
	inout[3] = 0x01;
	inout[4] = slot;
	_spi->enable();
	_spi->send(inout, 5);
	_spi->disable();
	_waitForBusy(MAX_TIMEOUT);
}

void EInk44::_uploadImageFixVal(int slot, bool white){
	#if EINK_INVERSE
	white = !white;
	#endif
	inout[0] = 0x20;
	inout[1] = 0x0B;
	inout[2] = slot;
	inout[3] = 0x01;
	inout[4] = (white) ? 0x00 : 0xFF;
	_spi->enable();
	_spi->send(inout, 5);
	_spi->disable();
	_waitForBusy(MAX_TIMEOUT);
}

void EInk44::_setImageROI(int x, int y, int w, int h)
{
	if(DEBUG) printf("Set ROI (%d, %d) @ (%d, %d).\n", w, h, x, y);
	inout[0] = 0x20;
	inout[1] = 0x0A;
	inout[2] = 0x00;
	inout[3] = 0x08;

	// X Min
	inout[4] = (x >> 8) & 0xFF;
	inout[5] = x & 0xFF;

	// X Max
	inout[6] = ((x + w) >> 8) & 0xFF;
	inout[7] = (x + w) & 0xFF;

	// Y Min
	inout[8] = (y >> 8) & 0xFF;
	inout[9] = y & 0xFF;

	// Y Max
	inout[10] = ((y + h) >> 8) & 0xFF;
	inout[11] = (y + h) & 0xFF;

	if(DEBUG){
		printf("ROI: ");
		printf("%x %x", inout[4], inout[5]);
		printf(" ");
		printf("%x %x", inout[6], inout[7]);
		printf(" ");
		printf("%x %x", inout[8], inout[9]);
		printf(" ");
		printf("%x %x", inout[10], inout[11]);
		printf("\n");
	}

	_spi->enable();
	_spi->send(inout, 12);
	_spi->disable();

	// if(!_readResponse()){
	// 	//printf("[EINK] [ERROR] Invalid set image ROI..\n");
	// }

	_waitForBusy(MAX_TIMEOUT);
}

}
