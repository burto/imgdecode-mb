#ifndef __SUBFILE__H
#define __SUBFILE__H

#include <sys/types.h>
#include <map>
#include <string>
#include "decoder.h"
#include "imgtypes.h"

using namespace std;

struct sec_info_struct {
	off_t offset;
	udword_t length;
	uword_t recsize;
};

typedef map<off_t,int> offset_list_t;

class subfile {
	class decoder *img;
	offset_list_t known_offsets;
	byte_t locked;
	uword_t hlen;
	string type;

public:
	subfile ();
	~subfile ();

	void subfile::init ();
	void subfile::decode_common_header ();

	void add_offset (off_t offset, int section);
	offset_list_t::iterator first_offset ();
	offset_list_t::iterator end_offset ();
};

// LBL Sections

#define LBL_LABELS      1
#define LBL_COUNTRY_DEF 2
#define LBL_REGION_DEF  3
#define LBL_CITY_DEF    4
#define LBL_UNKN1       5
#define LBL_POI_PROP    6
#define LBL_UNKN2       7
#define LBL_ZIP_DEF     8
#define LBL_HWY_DEF     9
#define LBL_EXIT_DEF    10
#define LBL_HWY_DATA    11
#define LBL_SORT_DESC   12
#define LBL_UNKN3       13
#define LBL_UNKN4       14

// TRE Sections

#define TRE_LEVELS      1
#define TRE_SUBDIV      2

#endif

