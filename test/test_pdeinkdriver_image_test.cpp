
#include <stdlib.h>
#include <assert.h>


#include <pdeinkdriver.h>
#include "pb.xbm"

PDEInkDriver::MAKE_XBM(pb);

int main(int argc, char* argv[]) 
{
	printf("Simple test running...\n");

	if (!PDEInkDriver::GPIO::GPIO_setup()) {
		warn("GPIO_setup failed");
		exit(2);
	}

	PDEInkDriver::EInk44 eink(
		PDEInkDriver::GPIO::GPIO_P9_16,
		PDEInkDriver::GPIO::GPIO_P9_15,
		PDEInkDriver::GPIO::GPIO_P9_25);

	eink.disable();
	usleep(50 * 1000);
	eink.enable();
	usleep(50 * 1000);

	printf("Erasing EINK...\n");
	eink.erase();
	eink.update();
	eink.waitUntilFree();

	printf("Filling EINK...\n");
	eink.fillROI(0, 0, 400, 300, false);
	eink.update();
	eink.waitUntilFree();
	sleep(3);

	printf("Example: Drawing an image and filling a region\n");
	eink.erase();
	eink.fillROI(0, pb.height(), EINK_WIDTH, EINK_HEIGHT - pb.height(), true);
	eink.sendImageROI(pb.bits(), 0, 0, pb.width(), pb.height());
	eink.update();
	sleep(3);

	printf("Example: Drawing an image inverted and inverted filling a region\n");
	eink.erase();
	eink.fillROI(0, pb.height(), EINK_WIDTH, EINK_HEIGHT - pb.height(), false);
	eink.sendImageROI(pb.bits(true), 0, 0, pb.width(), pb.height());
	eink.update();
	sleep(3);

	printf("Example: Partial updating a region.\n");
	eink.erase();
	eink.fillROI(0, pb.height(), EINK_WIDTH, EINK_HEIGHT - pb.height(), true);
	eink.copyImageROI(0, 0, pb.width(), pb.height());
	eink.updateFlashless();
	eink.waitUntilFree();

	printf("Example: Partial updating a region.\n");
	eink.erase();
	eink.copyImageROI(0, pb.height(), EINK_WIDTH, EINK_HEIGHT - pb.height());
	eink.sendImageROI(pb.bits(), 0, 0, pb.width(), pb.height());
	eink.updateFlashless();
	eink.waitUntilFree();

	printf("Example: Partial updating a region.\n");
	eink.erase();
	eink.copyImageROI(0, pb.height(), EINK_WIDTH, EINK_HEIGHT - pb.height());
	eink.sendImageROI(pb.bits(true), 0, 0, pb.width(), pb.height());
	eink.updateFlashless();
	eink.waitUntilFree();

	return 0;
}