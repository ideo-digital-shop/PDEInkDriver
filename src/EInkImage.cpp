
#include "EInkImage.h"

namespace PDEInkDriver {

EInkImage::EInkImage(int width, int height){
	_width = width;
	_height = height;
	image = (unsigned char*)malloc(length());
	createHeader();
	clear();
}

void EInkImage::createHeader()
{
   image[0] = 0x33;//panel type
   image[1] = 0x01;//x res
   image[2] = 0x90;
   image[3] = 0x01;//y res
   image[4] = 0x2c;
   image[5] = 0x01;//color depth
   image[6] = 0x00;//pixel data format - 0x02 optimized; 0x00 raw
   int i;
   for(i = 7; i < 16; i++)
      image[i] = 0x00;
}

unsigned char*  EInkImage::bits(){
	return image;
}

int EInkImage::length(){
	return 16 + _width * _height / 8;
}

void EInkImage::clear(bool white){
	if(white){
		memset(&image[16], 0xFF, length() - 16);
	} else {
		memset(&image[16], 0x00, length() - 16);	
	}
}

void EInkImage::addXBMImage(XBMImage& img){
	addXBMImage(img, 0, 0);
}

void EInkImage::addXBMImage(XBMImage& img, int start_x, int start_y)
{
	int y, x;
	for(y = 0; y < img.height(); y++){
		int row_index_src = (y * img.width()) / 8;
		int row_index_dst = ((y + start_y) * _width + start_x) / 8;
		for(x = 0; x < img.width() / 8; x++){
			if(x + start_x < _width){
				image[row_index_dst + x + 16] = img.bits()[row_index_src + x];
			}
		}
	}
}

void EInkImage::addXBMImage(XBMImage* img){
	addXBMImage(img, 0, 0);
}

void EInkImage::addXBMImage(XBMImage* img, int start_x, int start_y)
{
	int y, x;
	for(y = 0; y < img->height(); y++){
		int row_index_src = (y * img->width()) / 8;
		int row_index_dst = ((y + start_y) * _width + start_x) / 8;
		for(x = 0; x < img->width() / 8; x++){
			if(x + start_x < _width){
				image[row_index_dst + x + 16] = img->bits()[row_index_src + x];
			}
		}
	}
}

}