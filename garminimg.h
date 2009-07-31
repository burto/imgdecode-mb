#ifndef __GARMINIMG__H
#define __GARMINIMG__H

#include <sys/types.h>
#include <string>
#include <map>
#include <vector>
#include "imgtypes.h"

using namespace std;

class GarminImg;
class ImgFile;
class ImgSubfile;
class ImgTRE;
class ImgLBL;

struct img_fat_block_struct {
	string filename;
	string extension;
	string fullname;
	off_t offset;
	off_t size;
};
typedef struct img_fat_block_struct img_fat_block_t;

typedef map<string,img_fat_block_t> img_fat_t;
typedef map<string,class ImgSubfile *> subfile_list_t;
typedef map<string,class ImgFile *> file_list_t;
typedef map<uword_t,string> poly_name_t;
typedef map<udword_t,string> point_name_t;

class GarminImg {
	byte_t *ptr, *start, *bof, *save;
	string buffer, sbuffer;
	string fname;
	int fd;
	unsigned long bsize;
	file_list_t::iterator pfileent;
	poly_name_t polylines_byname, polygons_byname;
	point_name_t points_byname;
	poly_name_t marine_polylines_byname, marine_polygons_byname;
	point_name_t marine_points_byname;

	// Private utility methods

	void _xor (void *p, size_t len);

public:
	file_list_t files;
	img_fat_t fat;
	byte_t xorbyte;

	GarminImg ();
	~GarminImg ();

	// I/O methods

	int open (const char *path);
	int seek (off_t offset);
	int seek (off_t offset, int whence);
	off_t tell ();

	// Garmin data type methods

	byte_t get_byte ();
	word_t get_word ();
	uword_t get_uword ();
	dword_t get_dword ();
	udword_t get_udword ();
	dword_t get_int24 ();
	udword_t get_uint24 ();
	string get_string ();
	string get_string (size_t length);
	void get_cstring (void *dst, size_t length);
	time_t get_date ();
	string get_base11str (char delim);

	double degrees (dword_t units);
	double degrees (dword_t units, int bits);

	// General utility methods

	string base (long num, int base, int width);

	// Buffer functions

	string ibuffer(off_t *start_offset);

	void sbuffer_set ();
	void sbuffer_append ();
	void sbuffer_recall ();
	void sbuffer_swap ();
	void sbuffer_rtrim (size_t n);
	void sbuffer_ltrim (size_t n);
	off_t sbuffer_tell ();
	string::size_type sbuffer_size ();

	// IMG infrastructure methods

	void block_size_exp (unsigned int powoftwo);
	unsigned long block_size ();
	uword_t last_seq_block (off_t stopat);

	// IMG FAT/file methods

	class ImgFile *file_find (string name);
	void set_fileent ();
	class ImgFile *get_fileent ();

	// IMG element functions

	string elem_polyline_name (uword_t type);
	string elem_polygon_name (uword_t type);
	string elem_point_name (uword_t type, uword_t subtype);
	string elem_marine_polyline_name (uword_t type);
	string elem_marine_polygon_name (uword_t type);
	string elem_marine_point_name (uword_t type);
};

struct map_level_struct {
	byte_t bits;
	bool inherit;
	uword_t nsubdiv;
};
typedef struct map_level_struct map_level_t;

struct bound_struct {
	coord_t nw;
	coord_t se;
};
typedef struct bound_struct bound_t;

struct subdivision_struct {
	byte_t level;
	udword_t rgn_start;
	udword_t rgn_end;
	byte_t elements;
	coord_t center;
	bound_t boundary;
	uword_t last;
	uword_t next_level_idx;
	byte_t shiftby;
	udword_t ext_type_polygon_off;
	udword_t ext_type_polygon_len;
	udword_t ext_type_polyline_off;
	udword_t ext_type_polyline_len;
	udword_t ext_type_point_off;
	udword_t ext_type_point_len;
	byte_t ext_type_kinds;
};
typedef struct subdivision_struct map_subdivision_t;

typedef map<byte_t, map_level_t> levels_t;
typedef map<uword_t, map_subdivision_t> subdivisions_t;

struct locality_struct {
	string name;
	uword_t parent_idx;
};
typedef struct locality_struct locality_t;

typedef map<off_t,unsigned int> offset_list_t;
typedef map<off_t,string> label_cache_t;
typedef map<off_t,string> zip_cache_t;
typedef map<uword_t,string> country_cache_t;
typedef map<uword_t,locality_t> locality_cache_t;

// Master object that holds subfiles (RGN, LBL, TRE, etc.) and binds
// them together.

class ImgFile {
	subfile_list_t subfiles;
	offset_list_t section_offsets;
	label_cache_t labels;
	country_cache_t countries;
	locality_cache_t regions, cities;
	zip_cache_t zips;
	levels_t levels;
	subdivisions_t subdivisions;

public:
	class GarminImg *img;
	string name;

	ImgFile (class GarminImg *imgin, string namein);
	~ImgFile ();

	// Subfile methods

	void subfile_add (string type, off_t offset, udword_t size);
	class ImgSubfile *subfile_find (string type);
	off_t subfile_offset (string type);

	// Offset methods

	void offset_add (off_t offset, unsigned int section);
	off_t offset_next (off_t this_offset);
	unsigned int offset_find (off_t offset);

	// Label methods

	void label_store (off_t lbloffset, string label);
	string label_get (off_t lbloffset);
	string poi_get_name (off_t poioffset);
	vector<string> poi_get_address (off_t poioffset);

	// Locality methods

	void country_add (uword_t idx, string name);
	string country_get (uword_t idx);

	void region_add (uword_t idx, string name, uword_t cidx);
	string region_get (uword_t idx);
	vector<string> region_get_ar (uword_t idx);

	void city_add (uword_t idx, string name, uword_t ridx);
	string city_get (uword_t idx);
	vector<string> city_get_ar (uword_t idx);
	uword_t ncities ();

	void zip_add (uword_t idx, string name);
	string zip_get (uword_t idx);
	uword_t nzips ();

	// Level methods

	void level_add (byte_t zoom, bool inherited, byte_t bits,
		uword_t nsubdiv);
	int level_get (byte_t zoom, map_level_t *level);

	// Subdivision methods

	void subdivision_add (uword_t n, map_subdivision_t *subdiv);
	int subdivision_get (uword_t n, map_subdivision_t *subdiv);
	map_subdivision_t *subdivision_get (uword_t n);
	uword_t nsubdivisions ();
};

struct sec_info_struct {
	off_t offset;
	udword_t length;
	uword_t rsize;
};

class ImgSubfile {
	friend class ImgSRT;
	friend class ImgTRE;
	friend class ImgLBL;
	friend class ImgRGN;
	friend class ImgNET;
	friend class ImgNOD;

	off_t imgfile_offset;

public:
	class ImgFile *ifile;
	class GarminImg *img;
	uword_t hlen;
	udword_t size;
	bool locked;

	ImgSubfile ();
	~ImgSubfile ();

	// Offset methods

	off_t offset ();
};

// Subfile data structures

#define SUB_HAVE_POINT		0x10
#define SUB_HAVE_IDXPNT		0x20
#define SUB_HAVE_POLYLINE	0x40
#define SUB_HAVE_POLYGON	0x80

#define SUB_HAS_POINT(x) ((SUB_HAVE_POINT&(x))==SUB_HAVE_POINT)
#define SUB_HAS_IDXPNT(x) ((SUB_HAVE_IDXPNT&(x))==SUB_HAVE_IDXPNT)
#define SUB_HAS_POLYLINE(x) ((SUB_HAVE_POLYLINE&(x))==SUB_HAVE_POLYLINE)
#define SUB_HAS_POLYGON(x) ((SUB_HAVE_POLYGON&(x))==SUB_HAVE_POLYGON)
#define SUB_N_HAS(x) (SUB_HAS_POINT(x)+SUB_HAS_IDXPNT(x)+SUB_HAS_POLYLINE(x)+SUB_HAS_POLYGON(x))

// Subfile class defintions

class ImgNOD : public ImgSubfile {
public:
	struct sec_info_struct unknown1_info, unknown2_info, unknown3_info;
	int omult;

	ImgNOD (class ImgFile *ifilein, off_t offset);
	~ImgNOD ();
};

class ImgNET : public ImgSubfile {
public:
	struct sec_info_struct roads_info, unknown1_info, road_index_info;
	int omult;

	ImgNET (class ImgFile *ifilein, off_t offset);
	~ImgNET ();
};

class ImgRGN : public ImgSubfile {

	int _bits_per_coord (byte_t base, bool is_signed, bool extra_bit);
public:
	struct sec_info_struct data_info;
	struct sec_info_struct ext_type_polygons;
	struct sec_info_struct ext_type_polylines;
	struct sec_info_struct ext_type_points;

	ImgRGN (class ImgFile *ifilein, off_t offset);
	~ImgRGN ();

	// Bitstream functions

	void bits_per_coord (byte_t base, byte_t bfirst, bool extra_bit,
		int *blong, int *blat, int *sbits);
};

class ImgSRT : public ImgSubfile {

public:

	ImgSRT (class ImgFile *ifilein, off_t offset);
	~ImgSRT ();
};

class ImgTRE : public ImgSubfile {

public:
	struct sec_info_struct levels_info, subdiv_info, polyline_info,
		polygon_info, point_info, copyright_info, object_groups_info,
		ext_types_info;
	int nlevels, nsubdivisions;
	int num_ext_line_types, num_ext_area_types, num_ext_point_types;

	ImgTRE (class ImgFile *ifilein, off_t offset);
	~ImgTRE ();
};

#define LBL_ENC_6BIT	6
#define LBL_ENC_8BIT	9	// Yes, that's correct
#define LBL_ENC_10BIT	10

class ImgLBL : public ImgSubfile {
	const char *encoding6, *encoding6_shift, *encoding6_spec;

	// Private 6 bit label methods

	string char_6bit (byte_t byte, int *chset);
	string label_parse_6bit (off_t offset);

	string label_parse_8bit (off_t offset);

public:
	struct sec_info_struct country_info, region_info, city_info,
	  poiprop_info, zip_info, poi_index, poi_type_index, label_info,
		hwy_info;
	int omult, poiflags, ncities, nzips, encoding, sortlen;
	bool zipisphone;

	ImgLBL (class ImgFile *ifilein, off_t offset);
	~ImgLBL ();

	// Locality functions

	void country_def_parse (off_t offset);
	void region_def_parse (off_t offset);
	void city_def_parse (off_t offset);
	void postalcode_def_parse (off_t offset);

	// Label functions

	string label_parse_abs (off_t offset);
	string label_parse (off_t lbloffset);
};

// Subfile section offsets

#define TRE_LEVELS	0x0001
#define TRE_SUBDIV	0x0002
#define TRE_POLYLINE	0x0003
#define TRE_POLYGON	0x0004
#define TRE_POINT	0x0005
#define TRE_COPYRIGHT	0x0006
#define TRE_OBJECT_GROUPS 0x0007
#define TRE_EXT_TYPES	0x0008

#define	LBL_LABELS      0x1001
#define	LBL_COUNTRY_DEF	0x1002
#define	LBL_REGION_DEF  0x1003
#define	LBL_CITY_DEF    0x1004
#define	LBL_POI_INDEX   0x1005
#define	LBL_POI_PROP    0x1006
#define	LBL_POI_TYPE_INDEX 0x1007
#define	LBL_ZIP_DEF     0x1008
#define	LBL_HWY_DEF     0x1009
#define	LBL_EXIT_DEF    0x100A
#define	LBL_HWY_DATA    0x100B
#define	LBL_SORT_DESC   0x100C
#define	LBL_UNKN3       0x100D
#define	LBL_UNKN4       0x100E
#define	LBL_UNKN5       0x100F
#define	LBL_UNKN6       0x1010

#define NET_ROAD_DEF	0x2001
#define NET_ROAD_REC	0x2002
#define NET_UNKN1	0x2003
#define NET_ROAD_INDEX	0x2004

#define NOD_UNKN1	0x3001
#define NOD_UNKN2	0x3002

#endif

