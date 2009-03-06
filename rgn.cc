#include "garminimg.h"

ImgRGN::ImgRGN (class ImgFile *ifilein, off_t offset)
{
	ifile= ifilein;
	imgfile_offset= offset;
}

ImgRGN::~ImgRGN ()
{
}

//----------------------------------------------------------------------
// Bitstream functions
//----------------------------------------------------------------------

int ImgRGN::_bits_per_coord (byte_t base, bool is_signed, bool extra_bit)
{
	int n= 2;

	if ( base <= 9 ) n+= base;
	else n+= (2*base-9);

	if ( is_signed ) ++n;
	if ( extra_bit ) ++n;

	return n;
}

void ImgRGN::bits_per_coord (byte_t base, byte_t bfirst, bool extra_bit,
	int *blong, int *blat, int *sbits)
{
	bool x_sign_same, y_sign_same;
	bool sign_bit;

	// Get bits per longitude

	*sbits= 2;

	x_sign_same= bfirst & 0x1;

	if ( x_sign_same ) {
		sign_bit= 0;
		*sbits++;
	} else {
		sign_bit= 1;
	} 

	*blong= _bits_per_coord(base&0xF, sign_bit, extra_bit);

	// Get bits per latitude

	if ( x_sign_same ) y_sign_same= bfirst & 0x4;
	else y_sign_same= bfirst & 0x2;

	if ( y_sign_same ) {
		sign_bit= 0;
		*sbits++;
	} else {
		sign_bit= 1;
	} 

	*blat= _bits_per_coord((base&0xF0)>>4, sign_bit, extra_bit);
}

