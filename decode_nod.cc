#include "imgtypes.h"
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

static class GarminImg *img;
static class ImgFile *ifile;
static class ImgNOD *nod;
static class Decoder *dec;

void decode_nod_unknown1 ();
void decode_nod_unknown2 ();
void decode_nod_unknown3 ();

void decode_nod_header (class Decoder *dec_in, class ImgNOD *nod_in)
{
	udword_t length, offset;
	uword_t rsize;
	off_t soffset, eoffset;

	nod= nod_in;
	dec= dec_in;
	img= dec->img;
	ifile= nod->ifile;

	soffset= nod->offset();

	img->seek(soffset);

	dec->set_outfile("NOD", "header");
	dec->banner("NOD: Header");
	decode_common_header(dec, nod);

	dec->print("Unknown 1 at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Unknown 1 length %lu bytes",
		length= img->get_udword());
	nod->unknown1_info.offset= offset;
	nod->unknown1_info.length= length;

	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());

	dec->print("Unknown 2 at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Unknown 2 length %lu bytes",
		length= img->get_udword());
	nod->unknown2_info.offset= offset;
	nod->unknown2_info.length= length;

	dec->print("???", img->get_udword());

	dec->print("Unknown 3 at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Unknown 3 length %lu bytes",
		length= img->get_udword());
	dec->print("Unknown 3 record size %u bytes?",
		rsize= img->get_byte());
	nod->unknown3_info.offset= offset;
	nod->unknown3_info.length= length;
	nod->unknown3_info.rsize= rsize;

	(void) img->get_string(soffset+nod->hlen-img->tell());
	dec->print("???");
	dec->banner("NOD: End Header");
}

void decode_nod_body ()
{
	decode_nod_unknown1();

	decode_nod_unknown2();

	decode_nod_unknown3();
}

void decode_nod_unknown1 ()
{
	off_t soffset= nod->unknown1_info.offset;
	off_t eoffset= nod->unknown1_info.offset+nod->unknown1_info.length;

	img->seek(soffset);

	dec->set_outfile("NOD", "unknown1");
	dec->banner("NOD: Unknown section 1");

	dec->print("???", img->get_string(eoffset-soffset).c_str());
}

void decode_nod_unknown2 ()
{
	off_t soffset= nod->unknown2_info.offset;
	off_t eoffset= nod->unknown2_info.offset+nod->unknown2_info.length;

	img->seek(soffset);

	dec->set_outfile("NOD", "unknown2");
	dec->banner("NOD: Unknown section 2");

	while (img->tell() < eoffset) {
		dec->comment("Offset 0x%06x", img->tell()-soffset);
		dec->print("???", img->get_string(7).c_str());
	}
}

void decode_nod_unknown3 ()
{
	size_t nrecs= nod->unknown3_info.length/nod->unknown3_info.rsize;
	off_t soffset= nod->unknown3_info.offset;
	size_t n;

	img->seek(soffset);

	dec->set_outfile("NOD", "unknown3");
	dec->banner("NOD: Unknown section 3");

	double minLon = 360;
	double maxLon = 0;
	double minLat = 180;
	double maxLat = -180;
	for (n= 1; n<= nrecs; ++n) {
		dec->comment("Record %u", n);
		if(nod->unknown3_info.rsize == 9) {
		  int ilon =  img->get_uint24();
		  double lon = 360.0 * ilon / 0x01000000;
		  if(lon > maxLon) maxLon = lon;
		  if(lon < minLon) minLon = lon;
		  dec->print("Lon 0x%06x %f", ilon, lon);
		  int ilat =  img->get_uint24();
		  double lat = -180 + 360.0 * ilat / 0x01000000;
		  if(lat > maxLat) maxLat = lat;
		  if(lat < minLat) minLat = lat;
		  dec->print("Lat 0x%06x %f", ilat, lat);
		  dec->print("Point 0x%06x", img->get_uint24());
		}
		else
		  dec->print("???", img->get_string(nod->unknown3_info.rsize).c_str());
		dec->comment(NULL);
	}

	dec->comment("Lon min %f, max %f", minLon, maxLon);
	dec->comment("Lat min %f, max %f", minLat, maxLat);
}

