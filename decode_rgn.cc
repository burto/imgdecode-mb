#include "imgtypes.h"
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

static class GarminImg *img;
static class ImgFile *ifile;
static class ImgRGN *rgn;
static class ImgNET *net;
static class Decoder *dec;

void decode_rgn_subdiv (uword_t i, int *level);
void decode_tre_points (off_t oend, bool indexed);
void decode_tre_poly (off_t oend, bool line);

void decode_ext_type_polygons(udword_t off, udword_t len);
void decode_ext_type_polylines(udword_t off, udword_t len);
void decode_ext_type_points(udword_t off, udword_t len);

void decode_rgn_header (class Decoder *dec_in, class ImgRGN *rgn_in)
{
	off_t soffset;
	rgn= rgn_in;
	dec= dec_in;
	img= dec->img;
	ifile= rgn->ifile;

	net= (class ImgNET *) ifile->subfile_find("NET");

	soffset= rgn->offset();

	img->seek(soffset);

	dec->set_outfile("RGN", "header");
	dec->banner("RGN: Header");
	decode_common_header(dec, rgn);

	dec->print("Data at offset 0x%08x", 
		rgn->data_info.offset= img->get_udword()+soffset);
	dec->print("Data length %ld bytes", 
		rgn->data_info.length= img->get_udword());
	if(rgn->hlen > 29) {
	  dec->print("Ext Type Polygons at offset 0x%08x", 
		     rgn->ext_type_polygons.offset= img->get_udword()+soffset);
	  dec->print("Ext Type Polygons length %ld bytes", 
		     rgn->ext_type_polygons.length= img->get_udword());
	  dec->print("???", img->get_string(20).c_str());
	  dec->print("Ext Type Polylines at offset 0x%08x", 
		     rgn->ext_type_polylines.offset= img->get_udword()+soffset);
	  dec->print("Ext Type Polylines length %ld bytes", 
		     rgn->ext_type_polygons.length= img->get_udword());
	  dec->print("???", img->get_string(20).c_str());
	  dec->print("Ext Type Points at offset 0x%08x", 
		     rgn->ext_type_points.offset= img->get_udword()+soffset);
	  dec->print("Ext Type Points length %ld bytes", 
		     rgn->ext_type_points.length= img->get_udword());
	  dec->print("???", img->get_string(32).c_str());
	}
	if(rgn->hlen > 125)
	  dec->print("???", img->get_string(rgn->hlen - 125).c_str());
}

void decode_rgn_body ()
{
	int level;
	uword_t n= ifile->nsubdivisions();
	uword_t i;

	level= -1;
	for (i= 1; i<= n; ++i) decode_rgn_subdiv(i, &level);
}

void decode_rgn_subdiv (uword_t i, int *level)
{
	map_subdivision_t subdiv;
	off_t soffset, eoffset;
	string fname="level.";
	string banner;
	short nptrs;
	char snum[6], lnum[2];
	off_t opnt, oidx, opline, opgon, olast, oend;

	ifile->subdivision_get(i, &subdiv);

	if ( subdiv.elements == 0 ) return;
	if ( subdiv.rgn_start == subdiv.rgn_end ) return;

	soffset= rgn->data_info.offset+subdiv.rgn_start;
	eoffset= rgn->data_info.offset+subdiv.rgn_end;

	img->seek(soffset);

	if ( subdiv.level != *level ) {
		*level= subdiv.level;
		sprintf(lnum, "%02u", *level);
		fname+= lnum;
		dec->set_outfile("RGN", fname);
	}
	sprintf(snum, "%u", i);
	banner= "RGN: Subdivision ";
	banner+= snum;

	dec->banner(banner);

	opnt= oidx= opline= opgon= 0;
	nptrs= SUB_N_HAS(subdiv.elements)-1;
	olast= soffset + 2*nptrs;

	if ( SUB_HAS_POINT(subdiv.elements) ) {
		opnt= olast;
	}

	if ( SUB_HAS_IDXPNT(subdiv.elements) ) {
		if ( opnt ) {
			oidx= img->get_uword()+soffset;
			dec->print("Indexed points at 0x%08x", oidx);
		} else oidx= olast;
	} 

	if ( SUB_HAS_POLYLINE(subdiv.elements) ) {
		if ( opnt || oidx ) {
			opline= img->get_uword()+soffset;
			dec->print("Polylines at 0x%08x", opline);
		} else opline= olast;
	}

	if ( SUB_HAS_POLYGON(subdiv.elements) ) {
		if ( opnt || oidx || opline ) {
			opgon= img->get_uword()+soffset;
			dec->print("Polygons at 0x%08x", opgon);
		} else opgon= olast;
	}

	// Now decode our elements in order

	if ( SUB_HAS_POINT(subdiv.elements) ) {
		oend= oidx ? oidx : opline ? opline : opgon ? opgon : 
			eoffset;

		decode_tre_points(oend, false);
	}

	if ( SUB_HAS_IDXPNT(subdiv.elements) ) {
		oend= opline ? opline : opgon ? opgon : eoffset;

		decode_tre_points(oend, true);
	}

	if ( SUB_HAS_POLYLINE(subdiv.elements) ) {
		oend= opgon ? opgon : eoffset;

		decode_tre_poly(oend, true);
	}

	if ( SUB_HAS_POLYGON(subdiv.elements) ) {
		decode_tre_poly(eoffset, false);
	}

	if(subdiv.ext_type_kinds != 0) {
	  dec->comment("ExtType Kinds %d", subdiv.ext_type_kinds);
	  dec->comment(NULL);
	}

	if (subdiv.ext_type_polygon_len != 0) {
	  decode_ext_type_polygons(subdiv.ext_type_polygon_off, subdiv.ext_type_polygon_len);
	}
	if (subdiv.ext_type_polyline_len != 0) {
	  decode_ext_type_polylines(subdiv.ext_type_polyline_off, subdiv.ext_type_polyline_len);
	}
	if (subdiv.ext_type_point_len != 0) {
	  decode_ext_type_points(subdiv.ext_type_point_off, subdiv.ext_type_point_len);
	}
}

void decode_tre_points (off_t oend, bool indexed)
{
	uword_t idx= 1;

	dec->comment(NULL);
	if (indexed) dec->comment("Indexed Points to 0x%08x", oend);
	else dec->comment("Points to 0x%08x", oend);
	dec->comment(NULL);

	while ( img->tell() < oend ) {
		byte_t type, subtype;
		bool has_subtype = 0, is_poi;
		udword_t point_info, lbloffset;
		word_t dx, dy;
		string sinfo;

		if ( indexed ) dec->comment("Point #%u", idx++);
		type= img->get_byte();
		if ( has_subtype ) dec->print("Primary type %u", type);

		point_info= img->get_uint24();
		has_subtype= (point_info & 0x800000);
		is_poi=      (point_info & 0x400000);
		lbloffset=   (point_info & 0x1FFFFF);

		if ( is_poi ) {
			dec->print("POI offset 0x%06x in LBL", lbloffset);
			sinfo+= "is POI";
		} else 
			dec->print("label offset 0x%06x in LBL", lbloffset);

		if ( has_subtype ) {
			if ( is_poi ) sinfo+= ", has subtype";
			else sinfo= "has subtype";
		}

		if ( sinfo.size() ) dec->comment("%s", sinfo.c_str());

		dx= img->get_word();
		dec->print("long delta %d units (unshifted)", dx);
		dy= img->get_word();
		dec->print("lat delta %d units (unshifted)", dy);

		if ( has_subtype ) {
			subtype= img->get_byte();
			dec->print("Subtype %u", subtype);
		} else subtype= 0;

		dec->comment("%s", img->elem_point_name(type, subtype).c_str());
		if ( is_poi ) dec->comment("%s",
			ifile->poi_get_name(lbloffset).c_str());
		else dec->comment("%s", ifile->label_get(lbloffset).c_str());
		dec->comment(NULL);
	}
}

void decode_tre_poly (off_t oend, bool line)
{
	int n= 1;

	dec->comment(NULL);
	if (line) dec->comment("Polylines to 0x%08x", oend);
	else dec->comment("Polygons to 0x%08x", oend);
	dec->comment(NULL);

	while ( img->tell() < oend ) {
		byte_t type, bstream_info;
		word_t dx, dy;
		int bx, by, bs;
		uword_t bstream_len;
		bool extra_bit, two_byte_len, direction, lbl_in_net;
		udword_t lbloffset, lbl_info;
		string bitstream;

		if ( line ) dec->comment("Line %u", n++);
		type= img->get_byte();
		two_byte_len= (type & 0x80);
		if ( line ) {
			direction= (type & 0x40);
			type&= 0x3F;
			dec->print("Type %u, %s", type,
				img->elem_polyline_name(type).c_str());
			if ( direction ) dec->comment("One-way road");
			if ( two_byte_len ) dec->comment("2 byte length");
		} else {
			dec->print("Type %u, %s", type,
				img->elem_polygon_name(type).c_str());
		}

		lbl_info= img->get_uint24();
		lbloffset= lbl_info & 0x3FFFFF;
		extra_bit= lbl_info & 0x400000;
		if ( line ) {
			lbl_in_net= lbl_info & 0x800000;
			if ( lbl_in_net ) {
				off_t netoff= net->roads_info.offset+
					net->omult*lbloffset;
				ifile->offset_add(netoff, NET_ROAD_REC);
				dec->print("Label at NET offset 0x%06x",
					net->omult*lbloffset);
			} else {
				dec->print("Label at LBL offset 0x%06x",
					lbloffset);
				dec->comment("%s",
					ifile->label_get(lbloffset).c_str());
			}
		} else {
			dec->print("Label at LBL offset 0x%06x", lbloffset);
			dec->comment("%s",
				ifile->label_get(lbloffset).c_str());
		}
		if ( extra_bit ) dec->comment("extra bit in bitstream");

		dx= img->get_word();
		dec->print("pt#1 long delta %d units (unshifted)", dx);
		dy= img->get_word();
		dec->print("pt#1 lat delta %d units (unshifted)", dy);

		if ( two_byte_len ) {
			bstream_len= img->get_uword();
		} else {
			bstream_len= img->get_byte();
		}
		dec->print("Bitsteam %u bytes", bstream_len);

		bstream_info= img->get_byte();
		dec->print("Base %u/%u bits per long/lat",
			bstream_info&0xF, (bstream_info&0xF0)>>4);

		bitstream= img->get_string(bstream_len);
		rgn->bits_per_coord(bstream_info, bitstream[0],
			extra_bit, &bx, &by, &bs);

		dec->print("%u/%u bits per long/lat, %u setup", bx, by, bs,
			bitstream.c_str());
		dec->comment("%u point(s)", (8*bstream_len-bs)/(bx+by));
		dec->comment(NULL);
	}
}

void decode_ext_type_polygons(udword_t off, udword_t len) {
  dec->comment("ExtType areas off 0x%08x, len %u", off, len);
  dec->comment(NULL);
}

void decode_ext_type_polylines(udword_t off, udword_t len) {
  dec->comment("ExtType lines off 0x%08x, len %u", off, len);
  dec->comment(NULL);
}

void decode_ext_type_points(udword_t off, udword_t len) {

  dec->comment("ExtType points off 0x%08x, len %u", off, len);
  dec->comment(NULL);

  off_t start = rgn->ext_type_points.offset + off;
  off_t oend = start + len;
  img->seek(start);

  while ( img->tell() < oend ) {
    byte_t type, subtype;
    word_t dx, dy;

    type = img->get_byte();
    dec->print("Type 0x%02x", type);
    subtype = img->get_byte();
    bool has_label = (subtype & 0x20) != 0;
    bool unk1 = (subtype & 0x40) != 0;
    bool has_extra_byte = (subtype & 0x80) != 0;
    subtype &= 0x1f;
    dec->print("SubType 0x%02x", subtype);

    udword_t extType = 0x10000 | (type << 8) | subtype;
    dec->comment("Extended Type 0x%06x", extType);
    dec->comment("%s", img->elem_marine_point_name(extType & 0xffff).c_str());

    dx= img->get_word();
    dec->print("long delta %d units (unshifted)", dx);
    dy= img->get_word();
    dec->print("lat delta %d units (unshifted)", dy);

    if(has_label) {
      int lab_off = img->get_uint24();
      bool is_poi = (lab_off & 0x400000) != 0;
      lab_off &= 0x3ffff;
      if ( is_poi ) {
	dec->print("POI offset 0x%06x in LBL", lab_off);
      } else  {
	dec->print("label offset 0x%06x in LBL", lab_off);
      }
      if ( is_poi )
	dec->comment("%s", ifile->poi_get_name(lab_off).c_str());
      else
	dec->comment("%s", ifile->label_get(lab_off).c_str());
    }
    if(has_extra_byte) {
      byte_t extra = img->get_byte();
      dec->print("extra[0] 0x%02x", extra);
      if((extra & 0xe0) == 0x80) {
	dec->print("extra[1] 0x%02x", img->get_byte());
      }
      else if((extra & 0xe0) == 0xa0) {
	dec->print("extra[1] 0x%02x", img->get_byte());
	dec->print("extra[2] 0x%02x", img->get_byte());
      }
      else if((extra & 0xe0) == 0xe0) {
	byte_t extra1 = img->get_byte();
	dec->print("extra[1] 0x%02x", extra1);
	int n = extra1 >> 1;
	dec->print("extra[2..%d]", n + 1, img->get_string(n).c_str());
      }
      else if((extra & 0xe0) != 0) {
	dec->comment("*** Unrecognised extra[0] format ***");
      }
    }
    dec->comment(NULL);
  }

  if(img->tell() != oend) {
    dec->comment("*** OUT OF SYNC %u != %u ***", img->tell(), oend);
    dec->comment(NULL);
  }
}
