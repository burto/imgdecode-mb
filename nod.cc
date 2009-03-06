#include <sys/types.h>
#include <string>
#include "imgtypes.h"
#include "garminimg.h"

ImgNOD::ImgNOD (class ImgFile *ifilein, off_t offset)
{
	ifile= ifilein;
	imgfile_offset= offset;
}

ImgNOD::~ImgNOD ()
{
}

