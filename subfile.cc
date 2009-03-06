#include <sys/types.h>
#include <time.h>
#include "garminimg.h"

ImgSubfile::ImgSubfile ()
{
}

ImgSubfile::~ImgSubfile ()
{
}

off_t ImgSubfile::offset ()
{
	return imgfile_offset;
}

