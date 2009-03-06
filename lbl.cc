#include <sys/types.h>
#include <string>
#include "imgtypes.h"
#include "garminimg.h"
#include "lbl.h"

ImgLBL::ImgLBL (class ImgFile *ifilein, off_t offset)
{
	encoding6= " ABCDEFGHIJKLMNOPQRSTUVWXYZ~~~~~0123456789~~~~~~";
	encoding6_shift= "@!\"#$%&'()*+,-./~~~~~~~~~~:;<=>?~~~~~~~~~~~[\\]^_";
	encoding6_spec= "`abcdefghijklmnopqrstuvwxyz~~~~~0123456789~~~~~~";

	ifile= ifilein;
	imgfile_offset= offset;
}

ImgLBL::~ImgLBL ()
{
}

//-------------------------------------------------------------------
// Label methods
//-------------------------------------------------------------------

string ImgLBL::label_parse (off_t lbloffset)
{
	return label_parse_abs ( omult*lbloffset + label_info.offset );
}

string ImgLBL::label_parse_abs (off_t offset)
{
	switch (encoding) {
	case LBL_ENC_6BIT:
		return label_parse_6bit (offset);
		break;
	case LBL_ENC_8BIT:
		return label_parse_8bit (offset);
		break;
	}
}

//-------------------------------------------------------------------
// 6-bit decoding methods
//-------------------------------------------------------------------

string ImgLBL::label_parse_6bit (off_t offset)
{
	bool lcont= false;
	int chset= SET_6_NORMAL;
	unsigned char enc[3], data[4];
	string label;

	if ( offset > label_info.offset+label_info.length || 
		offset < label_info.offset ) {

		return "(invalid offset)";
	}

	if ( img->tell() != offset ) img->seek(offset);

	for (;;) {
		img->get_cstring(enc, 3);

		if ( lcont ) img->sbuffer_append();
		else img->sbuffer_set();

		data[0]= enc[0]>>2;

		if ( data[0] > 0x2F || (data[0] == 0 && ! lcont) ) {
			img->sbuffer_rtrim(2);
			img->seek(-2, SEEK_CUR);
			return label;
		}

		label+= char_6bit(data[0], &chset);

		data[1]= (enc[0]&0x03)<<4 | enc[1]>>4;
		if ( data[1] > 0x2F ) {
			img->sbuffer_rtrim(2);
			img->seek(-2, SEEK_CUR);
			return label;
		}

		label+= char_6bit(data[1], &chset);

		data[2]= (enc[1]&0x0F)<<2 | enc[2]>>6;
		if ( data[2] > 0x2F ) {
			img->sbuffer_rtrim(1);
			img->seek(-1, SEEK_CUR);
			return label;
		}

		label+= char_6bit(data[2], &chset);

		data[3]= enc[2]&0x3F;
		if ( data[3] > 0x2F ) return label;

		label+= char_6bit(data[3], &chset);
		lcont= true;
	}
}

string ImgLBL::char_6bit (byte_t byte, int *chset)
{
	string s;
	char ch;

	if ( *chset == SET_6_NORMAL ) {
		if ( byte == 0x1c ) {
			*chset= SET_6_SYMBOL;
			return "";
		} else if ( byte == 0x1b ) {
			*chset= SET_6_SPECIAL;
			return "<1b>";
		}

		ch= encoding6[byte];
	} else if ( *chset == SET_6_SYMBOL ) {
		ch= encoding6_shift[byte];
		*chset= SET_6_NORMAL;
	} else {
		ch= encoding6_spec[byte];
		*chset= SET_6_NORMAL;
	}

	if ( ch == '~' ) {
		char hex[16];
		sprintf(hex, "<%02x>", byte);
		return (string) hex;
	}

	s.append(&ch, 1);

	return s;
}

//-------------------------------------------------------------------
// 8-bit decoding methods
//-------------------------------------------------------------------

string ImgLBL::label_parse_8bit (off_t offset)
{
	bool lcont= false;
	int chset= SET_6_NORMAL;
	unsigned char enc[3], data[4];
	string label;

	if ( offset > label_info.offset+label_info.length || 
		offset < label_info.offset ) {

		return "(invalid offset)";
	}

	if ( img->tell() != offset ) img->seek(offset);

	for (;;) {
		img->get_cstring(enc, 1);

		img->sbuffer_set();

		if(enc[0] == 0)
		  return label;

		label += (char)enc[0];
	}
}


//-------------------------------------------------------------------
// Locality methods
//-------------------------------------------------------------------

