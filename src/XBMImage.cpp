
#include "XBMImage.h"

namespace PDEInkDriver {

unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

XBMImage::XBMImage(unsigned char * imgbits, int width, int height){
	_width = width;
	_height = height;
	_img = (unsigned char*)malloc(width * height / 8);
	_img_inverse = (unsigned char*)malloc(width * height / 8);
	int i;
	for(i = 0; i < width * height; i++){
		_img[i] = reverse(imgbits[i]);
		_img_inverse[i] = ~_img[i];
	}
} 

XBMImage::XBMImage(char * imgbits, int width, int height){
	_width = width;
	_height = height;
	_img = (unsigned char*)malloc(width * height / 8);
	_img_inverse = (unsigned char*)malloc(width * height / 8);
	int i;
	for(i = 0; i < width * height / 8; i++){
		_img[i] = reverse(imgbits[i]);
		_img_inverse[i] = ~_img[i];
	}
} 

XBMImage::~XBMImage(){
	if(_img){
		free(_img);
	}

	if(_img_inverse){
		free(_img_inverse);
	}
}

int XBMImage::length(){
	return 16 + _width * _height / 8;
}

int XBMImage::width(){
	return _width;
}

int XBMImage::height(){
	return _height;
}

unsigned char* XBMImage::bits(bool inverse){
	#ifdef EINK_INVERSE
	#if EINK_INVERSE
	if(inverse){
		return _img;
	} else {
		return _img_inverse;
	}
	#endif
	#else
	if(inverse){
		return _img_inverse;
	} else {
		return _img;
	}
	#endif
}

unsigned char* XBMImage::inverse(){
	#ifdef EINK_INVERSE
	#if EINK_INVERSE
	return _img;
	#endif
	#else
	return _img_inverse;
	#endif
}

}