#include <sys/types.h>
#include <string>
#include "imgtypes.h"
#include "garminimg.h"

ImgNET::ImgNET (class ImgFile *ifilein, off_t offset)
{
	ifile= ifilein;
	imgfile_offset= offset;
}

ImgNET::~ImgNET ()
{
}

