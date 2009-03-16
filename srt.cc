#include "garminimg.h"

ImgSRT::ImgSRT (class ImgFile *ifilein, off_t offset)
{
	ifile= ifilein;
	imgfile_offset= offset;
}

ImgSRT::~ImgSRT ()
{
}
