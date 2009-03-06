#include "imgtypes.h"
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

static class GarminImg *img;
static class ImgFile *ifile;
static class ImgTRE *tre;
static class Decoder *dec;

static void extract_remainder (off_t eoffset);
static void decode_tre_levels ();
static void decode_tre_subdivs ();
static void _decode_subdiv (map_subdivision_t *subdiv, byte_t shiftby);
static void decode_tre_polylines();
static void decode_tre_polygons();
static void decode_tre_points();
static void decode_tre_copyrights ();
static void decode_tre_unknown1 ();
static void decode_tre_unknown2 ();

void decode_tre_header (class Decoder *dec_in, class ImgTRE *tre_in)
{
	udword_t length, offset;
	uword_t rsize;
	off_t soffset, eoffset;
	class ImgLBL *lbl;

	tre= tre_in;
	dec= dec_in;
	img= dec->img;
	ifile= tre->ifile;

	soffset= tre->offset();
//	lbl= (class ImgLBL *) ifile->subfile_find("LBL");

	img->seek(soffset);

	dec->set_outfile("TRE", "header");
	dec->banner("TRE: Header");
	decode_common_header(dec, tre);

	dec->print("%10.5f North boundary", img->degrees(img->get_int24()));
	dec->print("%10.5f East boundary", img->degrees(img->get_int24()));
	dec->print("%10.5f South boundary", img->degrees(img->get_int24()));
	dec->print("%10.5f West boundary", img->degrees(img->get_int24()));

	dec->print("Map levels at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Map levels length %ld bytes",
		length= img->get_udword());
	ifile->offset_add(offset, TRE_LEVELS);
	tre->levels_info.offset= offset;
	tre->levels_info.length= length;
	tre->nlevels= length/4;

	dec->print("Subdivisions at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Subdivisions length %ld bytes",
		length= img->get_udword());
	ifile->offset_add(offset, TRE_SUBDIV);
	tre->subdiv_info.offset= offset;
	tre->subdiv_info.length= length;

	dec->print("Copyright at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Copyright length %ld bytes",
		length= img->get_udword());
	dec->print("Copyright records %u bytes",
		rsize= img->get_uword());
	ifile->offset_add(offset, TRE_COPYRIGHT);
	tre->copyright_info.offset= offset;
	tre->copyright_info.length= length;
	tre->copyright_info.rsize= rsize;

	dec->print("???", img->get_udword());
	dec->print("???", img->get_udword());
	dec->print("???", img->get_udword());
	dec->print("???", img->get_uword());
	dec->print("???", img->get_byte());

	dec->print("Polylines at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Polylines length %ld bytes",
		length= img->get_udword());
	dec->print("Polyline records %u bytes",
		rsize= img->get_uword());
	ifile->offset_add(offset, TRE_POLYLINE);
	tre->polyline_info.offset= offset;
	tre->polyline_info.length= length;
	tre->polyline_info.rsize= rsize;

	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());

	dec->print("Polygons at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Polygons length %ld bytes",
		length= img->get_udword());
	dec->print("Polygon records %u bytes",
		rsize= img->get_uword());
	ifile->offset_add(offset, TRE_POLYGON);
	tre->polygon_info.offset= offset;
	tre->polygon_info.length= length;
	tre->polygon_info.rsize= rsize;

	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());

	dec->print("Points at offset 0x%08x", 
		offset= img->get_udword()+soffset);
	dec->print("Points length %ld bytes",
		length= img->get_udword());
	dec->print("Point records %u bytes",
		rsize= img->get_uword());
	ifile->offset_add(offset, TRE_POINT);
	tre->point_info.offset= offset;
	tre->point_info.length= length;
	tre->point_info.rsize= rsize;

	dec->print("???", img->get_uword());
	dec->print("???", img->get_uword());

	eoffset= soffset+tre->hlen;
	if ( img->tell() < eoffset ) 
		dec->print("Map ID?", img->get_udword());

	// Only present in locked maps?

	if ( img->tell() < eoffset) {
		dec->print("???", img->get_udword());

		dec->print("Unknown1 at offset 0x%08x", 
			offset= img->get_udword()+soffset);
		dec->print("Unknown1 length %ld bytes",
			length= img->get_udword());
		dec->print("Unknown1 records %u bytes",
			rsize= img->get_uword());
		ifile->offset_add(offset, TRE_UNKN1);
		tre->unknown1_info.offset= offset;
		tre->unknown1_info.length= length;
		tre->unknown1_info.rsize= rsize;

		dec->print("???", img->get_udword());
		dec->print("???", img->get_udword());
		dec->print("???", img->get_udword());
		dec->print("???", img->get_udword());
		dec->print("???", img->get_udword());
		dec->print("???", img->get_string(20).c_str());

		dec->print("Unknown2 at offset 0x%08x", 
			offset= img->get_udword()+soffset);
		dec->print("Unknown2 length %ld bytes",
			length= img->get_udword());
		ifile->offset_add(offset, TRE_UNKN2);
		tre->unknown2_info.offset= offset;
		tre->unknown2_info.length= length;

		dec->print("???", img->get_string(eoffset-img->tell()).c_str());
	}

	dec->banner("TRE: End Header");
}

void decode_tre_body ()
{
	off_t soffset= tre->offset()+tre->hlen;
	off_t eoffset= tre->offset()+tre->size;
	off_t noffset;
	int type;

	// Parse the data segments in offset order.

	img->seek(soffset);

	// The map info and copyright strings

	type= ifile->offset_find(img->tell());
	if ( type == 0 ) {	// no registered offset here
		dec->set_outfile("TRE", "mapinfo");
		dec->banner("TRE: Map information");

		while ( type == 0 ) {
			noffset= ifile->offset_next(img->tell());
			if ( noffset > eoffset ) noffset= eoffset;

			dec->print("%s", img->get_string().c_str());

			type= ifile->offset_find(img->tell());
		}
	}

	// We can't just parse the TRE body linerarly.  Levels must
	// come before subdivisions, and those sections may not be
	// in the proper order (or even next to one another).  So,
	// we'll have to parse each known section individually and
	// in a particular order.

	// Parse levels first

	decode_tre_levels();

	// Next come subdivisions

	decode_tre_subdivs();

	// Object level information

	decode_tre_polylines();

	decode_tre_polygons();

	decode_tre_points();

	// Copyrights

	decode_tre_copyrights();

	// Unknown stuff

	decode_tre_unknown1();

	decode_tre_unknown2();
}

static void extract_remainder (off_t eoffset)
{
	off_t noffset;

	if ( ifile->offset_find(img->tell()) ) return; // No remainder

	noffset= ifile->offset_next(img->tell());
	if ( noffset > eoffset || ! noffset ) noffset= eoffset;

	dec->set_outfile("TRE", "unknown");
	dec->print("???", img->get_string(noffset-img->tell()).c_str());
}

static void decode_tre_levels ()
{
	int n= 0;

	img->seek(tre->levels_info.offset);
	dec->set_outfile("TRE", "levels");
	dec->banner("TRE: Level definitions");

	dec->comment("%u levels", tre->nlevels);
	dec->comment(NULL);

	for (n= 0; n< tre->nlevels; ++n) {
		byte_t zoom, bits, data;
		uword_t nsubdivisions;
		bool inherited;

		if ( tre->locked ) { 
			dec->print("Locked map level %u", n,
				img->get_string(4).c_str());
			continue;
		}

		data= img->get_byte();
		zoom= (data & 0xF);
		inherited= (data & 0x80);
		dec->print("Zoom level %u", zoom);
		dec->comment("Inherited: %c", (inherited) ? 'Y' : 'N');

		dec->print("%u bits per coordinate", bits= img->get_byte());
		dec->print("%u subdivisions", nsubdivisions=
			img->get_uword());
		dec->comment(NULL);
		tre->nsubdivisions+= nsubdivisions;

		ifile->level_add(zoom, inherited, bits, nsubdivisions);
	}
}

static void decode_tre_subdivs ()
{
	off_t eoffset= tre->subdiv_info.offset+tre->subdiv_info.length;
	int nchildren= 0;
	int ncsubdiv= 1;
	int recsz= 16;
	udword_t rgn_start, rgn_end;
	uword_t scount= 1;
	udword_t nsubdivisions;
	map_subdivision_t subdiv;
	int i, n;

	img->seek(tre->subdiv_info.offset);
	dec->set_outfile("TRE", "subdivs");
	dec->banner("TRE: Subdivision definitions");

	rgn_start= img->get_uint24();
	rgn_end= 0;

	if ( tre->locked ) {
		// We need to intuit the subdivision info

		int lcount= 1;
		int this_level;
		int next_level_at= 2;

		this_level= tre->nlevels - lcount;
		dec->comment("Map Level %u", this_level);
		dec->comment(NULL);
		while ( img->tell()+4 < eoffset ) {

			dec->comment("Subdivision %u", scount);

			if ( scount > 1 )
				dec->comment("Rgn start 0x%06x", rgn_start);
			else
				dec->print("Rgn start 0x%06x", rgn_start);

			_decode_subdiv(&subdiv, 0);

			subdiv.rgn_start= rgn_start;
			subdiv.level= this_level;

			if ( lcount < tre->nlevels ) {
				subdiv.next_level_idx= img->get_uword();
				dec->print("Next level at subdiv %u",
					subdiv.next_level_idx);
				if ( next_level_at == scount )
					next_level_at= subdiv.next_level_idx;
			} else {
				subdiv.next_level_idx= 0;
			}

			rgn_end= img->get_uint24();
			if ( img->tell() + 4 < eoffset )
				subdiv.rgn_end= (rgn_end) ? rgn_end-1 : 0;
			else subdiv.rgn_end= rgn_end;

			ifile->subdivision_add(scount, &subdiv);

			dec->print("Rgn end 0x%06x", subdiv.rgn_end);

			rgn_start= rgn_end;

			if ( next_level_at == scount+1 ) {
				++lcount;
				dec->comment(NULL);
				if ( lcount <= tre->nlevels )
					dec->comment("Map Level %u",
						(tre->nlevels - lcount));
				this_level= tre->nlevels - lcount;
			}

			dec->comment(NULL);

			++scount;
		}

		tre->nsubdivisions= scount-1;
	} else {
		dec->comment("%u subdivisions", tre->nsubdivisions);
		dec->comment(NULL);

		for (i= tre->nlevels-1; i>= 0; --i) {
			map_level_t level;

			recsz= ( i ) ? 16 : 14;
			ifile->level_get(i, &level);

			nsubdivisions= level.nsubdiv;

			dec->comment("Map Level %u", i);
			dec->comment(NULL);

			for (n= 0; n< nsubdivisions; ++n) {
				dec->comment("Subdivision %u", scount);

				if ( scount > 1 )
					dec->comment("Rgn start 0x%06x",
						rgn_start);
				else
					dec->print("Rgn start 0x%06x",
						rgn_start);

				_decode_subdiv(&subdiv, 24-level.bits);

				subdiv.rgn_start= rgn_start;
				subdiv.level= i;

				if ( recsz == 16 ) {
					subdiv.next_level_idx=
						img->get_uword();
					dec->print("Next level at subdiv %u",
						subdiv.next_level_idx);
				}

				rgn_end= img->get_uint24();
				if ( img->tell() + 4 < eoffset )
					subdiv.rgn_end= (rgn_end) ? rgn_end-1 : 0;
				else subdiv.rgn_end= rgn_end;
				ifile->subdivision_add(scount, &subdiv);

				dec->print("Rgn end 0x%06x", subdiv.rgn_end);

				rgn_start= rgn_end;

				dec->comment(NULL);

				++scount;
			}
		}
	}

	dec->print(NULL, img->get_byte());
}

static void _decode_subdiv (map_subdivision_t *subdiv, byte_t shiftby)
{
	uword_t width, height;
	udword_t rgninfo;
	string selems;

	subdiv->shiftby= shiftby;
	subdiv->rgn_end= 0xFFFFFFFF;

	subdiv->elements= img->get_byte();

	if ( SUB_HAS_POINT(subdiv->elements) ) selems+= " point";
	if ( SUB_HAS_IDXPNT(subdiv->elements) ) selems+= " idxpnt";
	if ( SUB_HAS_POLYLINE(subdiv->elements) ) selems+= " polyline";
	if ( SUB_HAS_POLYGON(subdiv->elements) ) selems+= " polygon";

	if ( selems.empty() ) dec->print("no map elements");
	else dec->print("has%s", selems.c_str());

	subdiv->center.ulong= img->get_int24();
	dec->print("Center %10.5f E", img->degrees(subdiv->center.ulong));
	subdiv->center.ulat= img->get_int24();
	dec->print("       %10.5f N", img->degrees(subdiv->center.ulat));

	width= img->get_uword();
	subdiv->last= (width & 0x8000);
	width&= 0x7FFF;

	if ( tre->locked ) {
		dec->print("width %u (unshifted) units", width);
	} else {
		width<<= shiftby;
		dec->print("width  %10.5f", img->degrees(width));
	}
	if ( subdiv->last ) dec->comment("last subdiv in chain");

	height= img->get_uword();
	if ( tre->locked ) {
		dec->print("height %u (unshifted) units", height);
	} else {
		height<<= shiftby;
		dec->print("height %10.5f", img->degrees(height));
	}

	if ( ! tre->locked ) {
		subdiv->boundary.nw.ulat  = subdiv->center.ulat+height;
		subdiv->boundary.nw.ulong = subdiv->center.ulong-width;
		subdiv->boundary.se.ulat  = subdiv->center.ulat-height;
		subdiv->boundary.se.ulong = subdiv->center.ulong+width;
	}
}

static void decode_tre_polylines()
{
	int nrecs= tre->polyline_info.length/tre->polyline_info.rsize;
	int n;

	img->seek(tre->polyline_info.offset);
	dec->set_outfile("TRE", "polylines");
	dec->banner("TRE: Polyline information");

	dec->comment("%u records", nrecs);

	for (n= 1; n<= nrecs; ++n) {
		byte_t type;

		dec->comment(NULL);
		type= img->get_byte();
		dec->print("Type %u, %s", type,
			img->elem_polyline_name(type).c_str());
		dec->print("max level %u?", img->get_byte());
		dec->print("???", img->get_byte());
	}
}

static void decode_tre_polygons()
{
	int nrecs= tre->polygon_info.length/tre->polygon_info.rsize;
	int n;

	img->seek(tre->polygon_info.offset);
	dec->set_outfile("TRE", "polygons");
	dec->banner("TRE: Polygon information");

	dec->comment("%u records", nrecs);

	for (n= 1; n<= nrecs; ++n) {
		byte_t type;

		dec->comment(NULL);
		type= img->get_byte();
		dec->print("Type %u, %s", type,
			img->elem_polygon_name(type).c_str());
		dec->print("max level %u?", img->get_byte());
		dec->print("???", img->get_byte());
	}
}

static void decode_tre_points()
{
	int nrecs= tre->point_info.length/tre->point_info.rsize;
	int n;

	img->seek(tre->point_info.offset);
	dec->set_outfile("TRE", "points");
	dec->banner("TRE: Point information");

	dec->comment("%u records", nrecs);

	for (n= 1; n<= nrecs; ++n) {
		byte_t type, subtype;

		dec->comment(NULL);
		dec->print("Type %u", type= img->get_byte());
		dec->print("max level %u?", img->get_byte());
		subtype= img->get_byte();
		dec->print("Subtype %u, %s", subtype,
			img->elem_point_name(type, subtype).c_str());
	}
}

static void decode_tre_copyrights ()
{
	int nrecs= tre->copyright_info.length/tre->copyright_info.rsize;
	int n;

	img->seek(tre->copyright_info.offset);
	dec->set_outfile("TRE", "copyright");
	dec->banner("TRE: Copyrights");

	for (n= 1; n<= nrecs; ++n) {
		udword_t lbloffset= img->get_uint24();
		dec->print("Label at 0x%06x", lbloffset);
		dec->comment("%s", ifile->label_get(lbloffset).c_str());
	}
}

static void decode_tre_unknown1 ()
{
	img->seek(tre->unknown1_info.offset);
	dec->set_outfile("TRE", "unknown1");
	dec->banner("TRE: Unknown section 1");

	dec->print("???", img->get_string(tre->unknown1_info.length).c_str());
}

static void decode_tre_unknown2 ()
{
	img->seek(tre->unknown2_info.offset);
	dec->set_outfile("TRE", "unknown2");
	dec->banner("TRE: Unknown section 2");

	dec->print("???", img->get_string(tre->unknown2_info.length).c_str());
}
