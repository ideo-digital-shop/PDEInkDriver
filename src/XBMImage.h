
#ifndef XBMIMAGE_H
#define XBMIMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include "globals.h"

namespace PDEInkDriver {

#define MAKE_XBM(_name)   \
	XBMImage _name(_name##_bits, _name##_width, _name##_height)


class XBMImage {

public:
	XBMImage(char* bits, int width, int height);
	XBMImage(unsigned char* bits, int width, int height);
	~XBMImage();
	int length();
	int width();
	int height();
	unsigned char* bits(bool inverse = false);
	unsigned char* inverse();

private:
	unsigned char* _img;
	unsigned char* _img_inverse;
	int _width;
	int _height;

};

}

#endif