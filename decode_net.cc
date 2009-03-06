#include <math.h>
#include "imgtypes.h"
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

static class GarminImg *img;
static class ImgFile *ifile;
static class ImgNET *net;
static class Decoder *dec;

void decode_net_roads ();
void decode_net_unkn2 ();

void decode_net_header (class Decoder *dec_in, class ImgNET *net_in)
{
	udword_t length, offset;
	uword_t rsize;
	off_t soffset, eoffset;

	net= net_in;
	dec= dec_in;
	img= dec->img;
	ifile= net->ifile;

	soffset= net->offset();

	img->seek(soffset);

	dec->set_outfile("NET", "header");
	dec->banner("NET: Header");
	decode_common_header(dec, net);

	dec->print("Road defs at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Road defs length %ld bytes",
		length= img->get_udword());
	ifile->offset_add(offset, NET_ROAD_DEF);
	net->roads_info.offset= offset;
	net->roads_info.length= length;

	dec->print("Road record offsets are x%u",
		net->omult= int(pow(2.0,img->get_byte())));

	dec->print("Unknown1 at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Unknown1 length %ld bytes",
		length= img->get_udword());
	dec->print(NULL, img->get_byte());
	ifile->offset_add(offset, NET_UNKN1);
	net->unknown1_info.offset= offset;
	net->unknown1_info.length= length;

	dec->print("Unknown2 at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Unknown2 length %ld bytes",
		length= img->get_udword());
	dec->print("Unknown2 record size %u bytes", 
		rsize= img->get_uword());
	ifile->offset_add(offset, NET_UNKN1);
	net->unknown2_info.offset= offset;
	net->unknown2_info.length= length;
	net->unknown2_info.rsize= rsize;

	dec->print("???", img->get_udword());
	dec->print("???", img->get_byte());
	dec->print("???", img->get_byte());
	dec->print("???", img->get_udword());
	dec->print("???", img->get_udword());
	dec->print("???", img->get_udword());

	dec->banner("NET: End Header");
}

void decode_net_body ()
{
	// Parse road information first.

	decode_net_roads ();

	// Unknow section 2.  I think these may be intersections.

	decode_net_unkn2 ();
}

void decode_net_roads ()
{
	// These have variable-length records that have not been
	// deciphered.  So, use the offsets that we created when
	// parsing RGN to determine the start of each record.

	off_t soffset= net->roads_info.offset;
	off_t eoffset= soffset+net->roads_info.length;

	img->seek(net->roads_info.offset);
	dec->set_outfile("NET", "roads");
	dec->banner("NET: Road definitions?");

	while ( img->tell() < eoffset ) {
		udword_t lbloffset, roadinfo;
		byte_t data;
		bool repeat;
		off_t noffset;
		int i, n;
		unsigned int otype;
		string stream;

		otype= ifile->offset_find(img->tell());
		if ( otype != NET_ROAD_REC && otype != NET_ROAD_DEF ) {
			fprintf(stderr, "Not a known offset at 0x%08x",
				img->tell());
			exit(1);
		}

		dec->comment(NULL);
		dec->comment("Road offset 0x%06x", img->tell()-soffset);

		repeat= true;
		while ( repeat ) {
			lbloffset= img->get_uint24();
			repeat= !(lbloffset & 0x800000);
			lbloffset&= 0x3FFFFF;
			dec->print("Label 0x%06x: %s", lbloffset,
				ifile->label_get(lbloffset).c_str());
		}

		data= img->get_byte();
		dec->print("Road data/flags %s?",
			img->base(data, 2, 8).c_str());
		dec->print("???", img->get_byte());
		dec->print("???", img->get_uword());

		repeat= true;
		n= 0;
		while ( repeat ) {
			data= img->get_byte();
			repeat= !(data&0x80);
			++n;
			dec->print("unknown, for idx %d?", n);
		}

		for (i= 0; i< n; ++i) {
			roadinfo= img->get_uint24();
			dec->print("Index %u, subdiv %u?",
				(roadinfo&0xFF), (roadinfo&0xFFFF00)>>8);
		}

		noffset= ifile->offset_next(img->tell());
		if ( noffset > eoffset || noffset == 0 ) noffset= eoffset;

		stream= img->get_string(noffset-img->tell());
		dec->print("%u byte stream", stream.length());
	}
}

void decode_net_unkn2 ()
{
	off_t soffset= net->unknown2_info.offset;
	off_t eoffset= soffset+net->unknown2_info.length;
	uword_t nrecs= net->unknown2_info.length/net->unknown2_info.rsize;
	uword_t i;

	img->seek(net->unknown2_info.offset);
	dec->set_outfile("NET", "unknown2");
	dec->banner("NET: Unknown Section 2 (Intersections?)");

	for (i= 1; i<= nrecs; ++i) {
		dec->comment("Record %u", i);

		dec->print("Offset 0x%06x ?", (img->get_uint24()&0x3FFFFF));
		dec->comment(NULL);
	}
}

