

#include <stdlib.h>
#include <assert.h>


#include <pdeinkdriver.h>


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
	sleep(1);

	printf("Filling EINK...\n");
	eink.fillROI(0, 0, 400, 300, false);
	eink.update();
	eink.waitUntilFree();

	return 0;
}