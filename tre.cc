#include "garminimg.h"

ImgTRE::ImgTRE (class ImgFile *ifilein, off_t offset)
{
	ifile= ifilein;
	imgfile_offset= offset;
	nsubdivisions= 0;
	nlevels= 0;
}

ImgTRE::~ImgTRE ()
{
}

//----------------------------------------------------------------------
// Level functions
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// Subdivision functions
//----------------------------------------------------------------------


