#include <math.h>
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

void decode_lbl_labels (off_t end);
void decode_lbl_country_def ();
void decode_lbl_region_def ();
void decode_lbl_city_def ();
void decode_lbl_zip ();
void decode_lbl_poi_index ();
void decode_lbl_poi_type_index ();
void decode_lbl_poiprop ();
void decode_lbl_hwy_def ();

static class GarminImg *img;
static class ImgFile *ifile;
static class Decoder *dec;
static class ImgLBL *lbl;

int n_highways;
int n_facilities;

void decode_lbl_header (class Decoder *dec_in, class ImgLBL *lbl_in)
{
	udword_t length;
	uword_t rsize;
	off_t offset;
	off_t soffset;

	lbl= lbl_in;
	dec= dec_in;
	img= dec->img;
	ifile= lbl->ifile;

	soffset= lbl->offset();

	img->seek(soffset);
	dec->set_outfile("LBL", "header");
	dec->banner("LBL: Header");

	decode_common_header(dec, lbl);

	offset= img->get_udword()+soffset;
	dec->print("Data offset 0x%06lx", offset);
	dec->print("Data length %lu bytes", length= img->get_udword());
	if (length) ifile->offset_add(offset, LBL_LABELS);

	dec->print("Label data offsets are x%u", 
		lbl->omult= int(pow(2.0,img->get_byte())));
	dec->print("Label coding %d", lbl->encoding= img->get_byte());
	lbl->label_info.length= length;
	lbl->label_info.offset= offset;

	offset= img->get_udword()+soffset;
	dec->print("Country definitions at 0x%06lx", offset);
	dec->print("Country definition length %lu bytes", 
		length= img->get_udword());
	dec->print("Country record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_COUNTRY_DEF);
	lbl->country_info.length= length;
	lbl->country_info.offset= offset;
	lbl->country_info.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Region definitions at 0x%06lx", offset);
	dec->print("Region definition length %lu bytes", 
		length= img->get_udword());
	dec->print("Region record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_REGION_DEF);
	lbl->region_info.length= length;
	lbl->region_info.offset= offset;
	lbl->region_info.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("City definitions at 0x%06lx", offset);
	dec->print("City definition length %lu bytes",
		length= img->get_udword());
	dec->print("City record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_CITY_DEF);
	lbl->city_info.length= length;
	lbl->city_info.offset= offset;
	lbl->city_info.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("POI Index at 0x%06lx", offset);
	dec->print("POI Index section length %lu bytes",
		length= img->get_udword());
	dec->print("POI Index record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_POI_INDEX);
	lbl->poi_index.length= length;
	lbl->poi_index.offset= offset;
	lbl->poi_index.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("POI properties at 0x%06lx", offset);
	dec->print("POI properties length %lu bytes",
		length= img->get_udword());
	dec->print("Zip bit is phone if no phone bit: %c", (lbl->zipisphone= img->get_byte()) ?
		'Y' : 'N');
	lbl->poiflags= img->get_byte();
	if ( lbl->poiflags ) {
		string s_flags;
		bool has_street, has_street_num, has_city,
			has_zip, has_phone, has_exit, has_u1, has_u2;

		has_street_num=	lbl->poiflags & 0x1;
		has_street=	lbl->poiflags & 0x2;
		has_city=	lbl->poiflags & 0x4;
		has_zip=	lbl->poiflags & 0x8;
		has_phone=	lbl->poiflags & 0x10;
		has_exit=	lbl->poiflags & 0x20;
		has_u1=		lbl->poiflags & 0x40;
		has_u2=		lbl->poiflags & 0x80;

		if ( lbl->zipisphone && has_zip && ! has_phone ) {
			has_zip= 0;
			has_phone= 1;
		}

		if ( has_street_num ) s_flags+= "street_num,";
		if ( has_street ) s_flags+= "street,";
		if ( has_city ) s_flags+= "city,";
		if ( has_zip ) s_flags+= "zip,";
		if ( has_phone ) s_flags+= "phone,";
		if ( has_exit ) s_flags+= "exit,";
		if ( has_u1 ) s_flags+= "unkn1,";
		if ( has_u2 ) s_flags+= "unkn2,";

		s_flags.erase(s_flags.size()-1);
		dec->print("%s POIs: %s", img->base(lbl->poiflags,
			2, 8).c_str(), s_flags.c_str());
	} else dec->print("No global POI properties");
	if (length) ifile->offset_add(offset, LBL_POI_PROP);
	lbl->poiprop_info.length= length;
	lbl->poiprop_info.offset= offset;

	dec->print("", img->get_uword());
	dec->print("", img->get_byte());

	offset= img->get_udword()+soffset;
	dec->print("POI Type Index section at 0x%06lx", offset);
	dec->print("POI Type Index section length %lu bytes",
		length= img->get_udword());
	dec->print("POI Type Index record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_POI_TYPE_INDEX);
	lbl->poi_type_index.length= length;
	lbl->poi_type_index.offset= offset;
	lbl->poi_type_index.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Zip definitions at 0x%06lx", offset);
	dec->print("Zip definition length %lu bytes",
		length= img->get_udword());
	dec->print("Zip record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_ZIP_DEF);
	lbl->zip_info.length= length;
	lbl->zip_info.offset= offset;
	lbl->zip_info.rsize= rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Highway definitions at 0x%06lx", offset);
	dec->print("Highway definition length %lu bytes",
		length= img->get_udword());
	dec->print("Highway record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_HWY_DEF);
	lbl->hwy_info.length= length;
	lbl->hwy_info.offset= offset;
	lbl->hwy_info.rsize= rsize;
	n_highways = length / rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Exit definitions at 0x%06lx", offset);
	dec->print("Exit definition length %lu bytes",
		length= img->get_udword());
	dec->print("Exit record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_EXIT_DEF);
	n_facilities = length / rsize;

	dec->print("", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Highway data definitions at 0x%06lx", offset);
	dec->print("Highway data definition length %lu bytes",
		length= img->get_udword());
	dec->print("Highway data record size %u bytes",
		rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_HWY_DATA);

	dec->print("", img->get_udword());

	dec->print("Code page?", img->get_uword());

	dec->print("???", img->get_udword());

	offset= img->get_udword()+soffset;
	dec->print("Sort descriptor at 0x%06lx", offset);
	dec->print("Sort descriptor length %lu bytes", 
		length= img->get_udword());
	if (length) ifile->offset_add(offset, LBL_SORT_DESC);
	lbl->sortlen= length;

	offset= img->get_udword()+soffset;
	dec->print("Unknown3 section at 0x%06lx", offset);
	dec->print("Unknown3 section length %lu bytes",
		length= img->get_udword());
	dec->print("Unknown3 record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_UNKN3);

	dec->print("", img->get_uword());

	offset= img->get_udword()+soffset;
	dec->print("Unknown4 section at 0x%06lx", offset);
	dec->print("Unknown4 section length %lu bytes",
		length= img->get_udword());
	dec->print("Unknown4 record size %u bytes", rsize= img->get_uword());
	if (length) ifile->offset_add(offset, LBL_UNKN4);

	dec->print("", img->get_uword());

//	offset= ifile->offset_next(img->tell());
	offset= soffset + lbl->hlen;
	if ( img->tell() < offset ) dec->print("???",
		img->get_string(offset - img->tell()).c_str());

	dec->banner("LBL: End Header");
}

void decode_lbl_body ()
{
	off_t soffset= lbl->offset()+lbl->hlen;
	off_t eoffset= lbl->offset()+lbl->size;
	off_t noffset;
	unsigned int type;

	// Parse the data segments in offset order.

	img->seek(soffset);
	type= ifile->offset_find(img->tell());
	if ( type == 0 ) {
		noffset= ifile->offset_next(img->tell());
		if ( noffset > eoffset ) noffset= eoffset;
		dec->print("???", img->get_string(noffset -
			img->tell()).c_str());
	}

	while ( img->tell() < eoffset ) {
		type= ifile->offset_find(img->tell());

		noffset= ifile->offset_next(img->tell());

		switch (type) {

		case 0:
			printf("Can't find section at offset 0x%x\n", img->tell());
			break;

		case LBL_LABELS:
			dec->set_outfile("LBL", "labels");
			dec->banner("LBL: Labels");
			decode_lbl_labels(img->tell()+lbl->label_info.length);
			break;
		case LBL_COUNTRY_DEF:
			dec->set_outfile("LBL", "countries");
			dec->banner("LBL: Country definitions");
			decode_lbl_country_def();
			break;
		case LBL_REGION_DEF:
			dec->set_outfile("LBL", "regions");
			dec->banner("LBL: Region definitions");
			decode_lbl_region_def();
			break;
		case LBL_CITY_DEF:
			dec->set_outfile("LBL", "cities");
			dec->banner("LBL: City definitions");
			decode_lbl_city_def();
			break;
		case LBL_POI_INDEX:
			dec->set_outfile("LBL", "poi_index");
			dec->banner("LBL: POI Index");
			decode_lbl_poi_index();
			break;
		case LBL_POI_PROP:
			dec->set_outfile("LBL", "poi_properties");
			dec->banner("LBL: POI Properties");
			decode_lbl_poiprop();
			break;
		case LBL_POI_TYPE_INDEX:
			dec->set_outfile("LBL", "poi_type_index");
			dec->banner("LBL: POI Type Index");
			decode_lbl_poi_type_index();
			break;
		case LBL_ZIP_DEF:
			dec->set_outfile("LBL", "zipcodes");
			dec->banner("LBL: Zip/Postal codes");
			decode_lbl_zip();
			break;
		case LBL_HWY_DEF:
			dec->set_outfile("LBL", "highways");
			dec->banner("LBL: Highways");
			decode_lbl_hwy_def();
			break;
		case LBL_EXIT_DEF:
			dec->set_outfile("LBL", "exits");
			dec->banner("LBL: Exit services");
			break;
		case LBL_HWY_DATA:
			dec->set_outfile("LBL", "hwydata");
			dec->banner("LBL: Highway data");
			break;
		case LBL_SORT_DESC:
			dec->set_outfile("LBL", "sort");
			dec->banner("LBL: Sort descriptor");
			dec->print("%s", img->get_string(lbl->sortlen).c_str());
			break;
		case LBL_UNKN3:
			dec->set_outfile("LBL", "unknown3");
			dec->banner("LBL: Unknown 3");
			break;
		case LBL_UNKN4:
			dec->set_outfile("LBL", "unknown4");
			dec->banner("LBL: Unknown 4");
			break;
		}

		if ( noffset > eoffset || noffset == 0 ) noffset= eoffset;
		if ( img->tell() < noffset ) {
			dec->print("???", img->get_string(noffset
				- img->tell()).c_str());

		}
	}
}

void decode_lbl_labels (off_t end)
{
	off_t offset;
	string label;

	offset= img->tell();
	while ( offset < end ) {
		int n;
		off_t lbloffset;

		label= lbl->label_parse_abs(offset);

		lbloffset= (offset - lbl->label_info.offset)/lbl->omult;
		dec->comment("Offset 0x%06x", lbloffset);

		ifile->label_store(lbloffset, label);

		img->sbuffer_recall();
		dec->print("%s", label.c_str());

		offset= img->tell();

		if ( lbl->omult > 1 ) {
			n= (offset-lbl->label_info.offset) % lbl->omult;

			if ( n ) {
				dec->print(NULL,
					img->get_string(lbl->omult-n).c_str());
				offset= img->tell();
			}
		}
		dec->comment(NULL);

	}
}

void decode_lbl_country_def ()
{
	int nrecs= lbl->country_info.length / lbl->country_info.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		off_t loffset;
		string country;

		loffset= img->get_uint24();
		dec->print("Country %u: LBL offset 0x%06x", n, loffset);

		country= ifile->label_get(loffset);
		dec->comment("%s", country.c_str());
//		img->country(n, &label);
	}
	dec->comment(NULL);
}


void decode_lbl_region_def ()
{
	int nrecs= lbl->region_info.length / lbl->region_info.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		uword_t cidx;
		udword_t loffset;
		string label;

		cidx= img->get_uword();
		dec->print("In country %u", cidx);

		loffset= img->get_uint24();
		dec->print("Region %u: LBL offset 0x%06x", n, loffset);

		label= ifile->label_get(loffset);
		dec->comment("%s", label.c_str());

		ifile->region_add(n, label, cidx);
	}
}

void decode_lbl_city_def ()
{
	int nrecs= lbl->city_info.length / lbl->city_info.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		udword_t cdata;
		uword_t cinfo, ridx;
		bool pref;
		string label;

		cdata= img->get_uint24();
		img->sbuffer_set();

		cinfo= img->get_uword();
		img->sbuffer_swap();

		ridx= (cinfo & 0x3FFF);
		pref= (cinfo & 0x8000);
		
		if ( pref ) {
			dec->print("City %u is IdxPoint %u in subdiv %u",
				   n, cdata&0xff, cdata>>8);
			label = "(CITY NAMED BY INDEXED POINT)";
		}
		else {
			udword_t loffset= (udword_t) cdata;

			dec->print("City %u label at 0x%06x", n, loffset);
			label= ifile->label_get(loffset);
		}
		ifile->city_add(n, label, ridx);

		img->sbuffer_recall();
		dec->print("in region %u", ridx);
		if ( pref ) dec->comment("is point offset");
		else dec->comment("%s", ifile->city_get(n).c_str());
		dec->comment(NULL);
	}
}

void decode_lbl_zip ()
{
	int nrecs= lbl->zip_info.length / lbl->zip_info.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		udword_t offset= img->get_uint24();
		string label;
		
		dec->print("Zip %u label at 0x%06x", n, offset);
		dec->comment("%s", ifile->label_get(offset).c_str());
		ifile->zip_add(n, label);
	}
}

void decode_lbl_poi_index ()
{
	int nrecs= lbl->poi_index.length / lbl->poi_index.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		byte_t poi_index = img->get_byte();
		uword_t poi_group_index = img->get_uword();
		byte_t poi_sub_type = img->get_byte();

		dec->print("Entry %u is POI %u, Grp %u, SubType 0x%0.2x",
			   n, poi_index, poi_group_index, poi_sub_type);
	}
}

void decode_lbl_poi_type_index ()
{
	int nrecs= lbl->poi_type_index.length / lbl->poi_type_index.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
		byte_t type = img->get_byte();
		udword_t start_index= img->get_uint24();

		dec->print("Entry %u is Type 0x%0.2x, Start Index %u",
			   n, type, start_index);
	}
}

void decode_lbl_poiprop ()
{
	off_t soffset= img->tell();
	off_t eos= img->tell() + lbl->poiprop_info.length;
	udword_t poi_data, lbloffset;
	bool override;

	while ( img->tell() < eos ) {
		byte_t flags;
		bool has_street, has_street_num, has_city,
			has_zip, has_phone, has_exit, has_u1, has_u2;

		poi_data= img->get_uint24();
		lbloffset= (poi_data & 0x3FFFFF);
		override= (poi_data & 0x800000);

		dec->print("Label offset 0x%06x", lbloffset);
		dec->comment("%s", ifile->label_get(lbloffset).c_str());
		if ( override ) {
			byte_t oflags= img->get_byte();
			flags = 0;
			int bit = 1;
			for(int i = 0; i < 8; ++i) {
			  while(!(lbl->poiflags & bit) && (bit < 256))
			    bit <<= 1;
			  if(oflags & (1 << i))
			    flags |= bit;
			  bit <<= 1;
			}

			dec->print("%s override flags -> %s",
				   img->base(oflags, 2, 8).c_str(),
				   img->base(flags, 2, 8).c_str());

		} else flags= lbl->poiflags;

		has_street_num=	flags & 0x1;
		has_street=	flags & 0x2;
		has_city=	flags & 0x4;
		has_zip=	flags & 0x8;
		has_phone=	flags & 0x10;
		has_exit=	flags & 0x20;
		has_u1=		flags & 0x40;
		has_u2=		flags & 0x80;

/*
		if ( lbl->zipisphone && has_zip && ! has_phone ) {
			has_zip= 0;
			has_phone= 1;
		}
*/

		if ( override ) {
			string s_flags;

			if ( has_street_num ) s_flags+= "street_num,";
			if ( has_street ) s_flags+= "street,";
			if ( has_city ) s_flags+= "city,";
			if ( has_zip ) s_flags+= "zip,";
			if ( has_phone ) s_flags+= "phone,";
			if ( has_exit ) s_flags+= "exit,";
			if ( has_u1 ) s_flags+= "unkn1,";
			if ( has_u2 ) s_flags+= "unkn2,";

			if ( s_flags.size() )
				s_flags.erase(s_flags.size()-1);
			dec->print("Has %s", s_flags.c_str());
		}

		if ( has_street_num ) {
			string snum= img->get_base11str('-');
			if ( snum.empty() ) {
				udword_t mpoffset;
				string::size_type idx;

				dec->print("street num is label");
				mpoffset= img->get_byte()<<16;
				img->sbuffer_set();
				mpoffset|= img->get_uword();
				img->sbuffer_append();
				img->sbuffer_recall();
				dec->print("label at 0x%06x", mpoffset);
				snum= ifile->label_get(mpoffset);
				idx= snum.find("-6", 0);
				if ( idx != string::npos ) 
					snum.replace(idx, 2, ", Suite ");
				dec->comment("Address %s", snum.c_str());
			} else dec->print("Street num %s", snum.c_str());
		}

		if ( has_street ) {
			lbloffset= img->get_uint24();
			dec->print("Street lbl at 0x%06x", lbloffset);
			dec->comment("%s", ifile->label_get(lbloffset).c_str());
		}

		if ( has_city ) {
			udword_t cidx;

			if ( ifile->ncities() > 0xFF ) cidx= img->get_uword();
			else cidx= img->get_byte();

			dec->print("City idx %lu", cidx);
			dec->comment("%s", ifile->city_get(cidx).c_str());
		}

		if ( has_zip ) {
			udword_t zidx;

			if ( ifile->nzips() > 0xFF ) zidx= img->get_uword();
			else zidx= img->get_byte();

			dec->print("Zip idx %lu", zidx);
			dec->comment("%s", ifile->zip_get(zidx).c_str());
		}

		if ( has_phone ) {
			string phn= img->get_base11str('-');
			if ( phn.size() )
				dec->print("Ph num %s", phn.c_str());
		}

		if ( has_exit ) {
			lbloffset= img->get_uint24();
			int overnightParking = (lbloffset & 0x400000) != 0;
			int exitFacilities = (lbloffset & 0x800000) != 0;
			lbloffset &= 0x3ffff;
			dec->print("Exit lbl at 0x%06x", lbloffset);
			dec->comment("%s", ifile->label_get(lbloffset).c_str());
			dec->print("overnight parking %d, have facilites %d", overnightParking, exitFacilities);
			int n;
			if ( n_highways > 0xFF ) n= img->get_uword();
			else n= img->get_byte();
			dec->print("Highway idx %lu", n);
			if(exitFacilities) {
			  if ( n_facilities > 0xFF ) n= img->get_uword();
			  else n= img->get_byte();
			  dec->print("Exit facility idx 1 %lu", n);
			}
		}

		while ( (img->tell()-soffset) % lbl->omult &&
			img->tell() < eos ) {

			dec->print(NULL, img->get_byte());
		}

		dec->comment(NULL);
	}

	if ( img->tell() > eos ) {
		fprintf(stderr, "POI prop overrun! eos = 0x%x, curPos = 0x%x\n",
			eos, img->tell());
		img->seek(eos);
	}
}

void decode_lbl_hwy_def ()
{
	int nrecs= lbl->hwy_info.length / lbl->hwy_info.rsize;
	int n;

	for (n= 1; n<= nrecs; ++n) {
	  //		dec->print("???", img->get_string(lbl->region_info.rsize).c_str());
		uword_t cidx;
		udword_t loffset;
		string label;

		loffset= img->get_uint24();
		dec->print("Highway %u: LBL offset 0x%06x", n, loffset);

		label= ifile->label_get(loffset);
		dec->comment("%s", label.c_str());

		cidx= img->get_uword();
		dec->print("Offset in Highway Data %u", cidx);
		dec->print("Unknown", img->get_byte());
	}
}

