

#ifndef EINK_IMAGE_H
#define EINK_IMAGE_H

#include "XBMImage.h"

namespace PDEInkDriver {

class EInkImage {

public:
	EInkImage(int width, int height);

	int length();

	void clear(bool white = false);
	void createHeader();

	void addXBMImage(XBMImage& img);
	void addXBMImage(XBMImage& img, int start_x, int start_y);

	void addXBMImage(XBMImage* img);
	void addXBMImage(XBMImage* img, int start_x, int start_y);

	unsigned char* bits();

private:
	int _height;
	int _width;
	unsigned char* image;
};

}

#endif