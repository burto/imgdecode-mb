#include "garminimg.h"

using namespace std;

ImgFile::ImgFile (class GarminImg *imgin, string namein)
{
	img= imgin;
	name= namein;
}

ImgFile::~ImgFile ()
{
	subfile_list_t::iterator sfpos;

	for (sfpos= subfiles.begin(); sfpos != subfiles.end(); ++sfpos)
		delete sfpos->second;
}

//----------------------------------------------------------------------
// Subfile methods
//----------------------------------------------------------------------

void ImgFile::subfile_add (string type, off_t offset, udword_t sz)
{
	class ImgSubfile *subfile= NULL;

	if ( type == "TRE" ) {
		subfile= new ImgTRE (this, offset);
	} else if ( type == "LBL" ) {
		subfile= new ImgLBL (this, offset);
	} else if ( type == "RGN" ) {
		subfile= new ImgRGN (this, offset);
	} else if ( type == "NET" ) {
		subfile= new ImgNET (this, offset);
	} else if ( type == "NOD" ) {
		subfile= new ImgNOD (this, offset);
	} else if ( type == "SRT" ) {
		subfile= new ImgSRT (this, offset);
	} else {
		fprintf(stderr, "Unknown subfile %s\n", type.c_str());
		return;
	}

	subfile->size= sz;
	subfile->img= img;
	subfiles.insert(make_pair(type, subfile));
}

class ImgSubfile *ImgFile::subfile_find (string type)
{
	subfile_list_t::iterator sfpos= subfiles.find(type);

	if ( sfpos == subfiles.end() ) return NULL;

	return sfpos->second;
}

off_t ImgFile::subfile_offset (string type)
{
	class ImgSubfile *subfile= subfile_find(type);
	if ( subfile == NULL ) return 0;

	return subfile->offset();
}

//----------------------------------------------------------------------
// Offset methods
//----------------------------------------------------------------------

void ImgFile::offset_add (off_t offset, unsigned int section)
{
	section_offsets.insert(make_pair(offset, section));
}

off_t ImgFile::offset_next (off_t this_offset)
{
	offset_list_t::iterator opos;

	opos= section_offsets.upper_bound(this_offset);
	if ( opos == section_offsets.end() ) return 0;

	return opos->first;
}

unsigned int ImgFile::offset_find (off_t offset)
{
	offset_list_t::iterator opos= section_offsets.find(offset);

	if ( opos == section_offsets.end() ) return 0;

	return opos->second;
}

//----------------------------------------------------------------------
// Label methods
//----------------------------------------------------------------------

void ImgFile::label_store (off_t lbloffset, string label) 
{
	labels.insert(make_pair(lbloffset, label));
}

string ImgFile::label_get (off_t lbloffset)
{
	label_cache_t::iterator lpos;

	lpos= labels.find(lbloffset);
	if ( lpos == labels.end() ) {
		string label;
		class ImgLBL *lbl= (class ImgLBL *) subfile_find("LBL");

		if ( lbl != NULL ) {
			off_t here= img->tell();

			img->sbuffer_set();
			label= lbl->label_parse(lbloffset);
			img->sbuffer_recall();
			img->seek(here);
			label_store(lbloffset, label);
		} else return "";

		return label;
	}

	return lpos->second;
}

string ImgFile::poi_get_name (off_t poioffset)
{
	class ImgLBL *lbl= (class ImgLBL *) subfile_find("LBL");
	udword_t lbloffset;
	off_t here= img->tell();

	if ( lbl == NULL ) return "";

	img->sbuffer_set();
	img->seek(lbl->poiprop_info.offset + lbl->omult*poioffset);

	lbloffset= img->get_uint24() & 0x3FFFFF;

	img->sbuffer_recall();
	img->seek(here);

	return label_get(lbloffset);
}


//----------------------------------------------------------------------
// Locality methods
//----------------------------------------------------------------------

void ImgFile::country_add (uword_t idx, string name)
{
	countries.insert(make_pair(idx, name));
}

string ImgFile::country_get (uword_t idx)
{
	country_cache_t::iterator cpos;

	cpos= countries.find(idx);
	if ( cpos == countries.end() ) return "";

	return cpos->second;
}

void ImgFile::region_add (uword_t idx, string name, uword_t cidx)
{
	locality_t region;

	region.name= name;
	region.parent_idx= cidx;

	regions.insert(make_pair(idx, region));
}

string ImgFile::region_get (uword_t idx)
{
	locality_cache_t::iterator rpos;

	rpos= regions.find(idx);
	if ( rpos == regions.end() ) return "";

	return rpos->second.name;
}

vector<string> ImgFile::region_get_ar (uword_t idx)
{
	locality_cache_t::iterator rpos;
	vector<string> region;

	rpos= regions.find(idx);
	if ( rpos == regions.end() ) return region;

	region.push_back(rpos->second.name);
	region.push_back(country_get(rpos->second.parent_idx));

	return region;
}

void ImgFile::city_add (uword_t idx, string name, uword_t ridx)
{
	locality_t city;

	city.name= name;
	city.parent_idx= ridx;

	cities.insert(make_pair(idx, city));
}

string ImgFile::city_get (uword_t idx)
{
	locality_cache_t::iterator cpos;

	cpos= cities.find(idx);
	if ( cpos == cities.end() ) return "";

	return cpos->second.name;
}

vector<string> ImgFile::city_get_ar (uword_t idx)
{
	locality_cache_t::iterator cpos;
	vector<string> city, region;

	city.reserve(3);

	cpos= cities.find(idx);
	if ( cpos == cities.end() ) return city;

	city.push_back(cpos->second.name);

	region= region_get_ar(cpos->first);
	city.push_back(region[0]);
	city.push_back(region[1]);

	return city;
}

uword_t ImgFile::ncities ()
{
	return cities.size();
}

void ImgFile::zip_add (uword_t idx, string zip)
{
	zips.insert(make_pair(idx, zip));
}

string ImgFile::zip_get (uword_t idx)
{
	zip_cache_t::iterator zpos;

	zpos= zips.find(idx);
	if ( zpos == zips.end() ) return "";

	return zpos->second;
}

uword_t ImgFile::nzips()
{
	return zips.size();
}

//----------------------------------------------------------------------
// Map level methods
//----------------------------------------------------------------------

void ImgFile::level_add (byte_t zoom, bool inherited, byte_t bits, 
	uword_t nsubdiv)
{
	map_level_t level;

	level.inherit= inherited;
	level.bits= bits;
	level.nsubdiv= nsubdiv;

	levels.insert(make_pair(zoom, level));
}

int ImgFile::level_get (byte_t zoom, map_level_t *level)
{
	levels_t::iterator lpos;

	lpos= levels.find(zoom);
	if ( lpos == levels.end() ) return -1;

	*level= lpos->second;
	return 0;
}

//----------------------------------------------------------------------
// Map subdivision methods
//----------------------------------------------------------------------

void ImgFile::subdivision_add (uword_t n, map_subdivision_t *subdiv)
{
	subdivisions.insert(make_pair(n, *subdiv));
}

int ImgFile::subdivision_get (uword_t n, map_subdivision_t *subdiv)
{
	subdivisions_t::iterator spos;

	spos= subdivisions.find(n);
	if ( spos == subdivisions.end() ) return -1;

	*subdiv= spos->second;
	return 0;
}

map_subdivision_t *ImgFile::subdivision_get (uword_t n)
{
	subdivisions_t::iterator spos;

	spos= subdivisions.find(n);
	if ( spos == subdivisions.end() ) return NULL;

	return &spos->second;
}

uword_t ImgFile::nsubdivisions ()
{
	return subdivisions.size();
}

