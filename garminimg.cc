#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/mman.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <string>
#include <algorithm>
#include "garminimg.h"
#include "elemdata.h"

using namespace std;

struct fat_block_struct {
	string filename;
	string extension;
	string fullname;
	off_t size;
	off_t offset;
};
typedef struct fat_block_struct fat_block_t;

extern int errno;

GarminImg::GarminImg ()
{
	const struct elem_info_struct *eptr;
	xorbyte= 0;
	fd= -1;
	ptr= NULL;

	// Load our element types

	for (eptr= polyline_data; eptr->type; ++eptr) 
		polylines_byname.insert(make_pair(eptr->type, eptr->name));

	for (eptr= polygon_data; eptr->type; ++eptr) 
		polygons_byname.insert(make_pair(eptr->type, eptr->name));

	for (eptr= point_data; eptr->type; ++eptr) 
		points_byname.insert(make_pair(eptr->type, eptr->name));

	for (eptr= marine_point_data; eptr->type; ++eptr) 
		marine_points_byname.insert(make_pair(eptr->type, eptr->name));
}

GarminImg::~GarminImg ()
{
	file_list_t::iterator fpos;

	for (fpos= files.begin(); fpos != files.end(); ++fpos)
		delete fpos->second;

	if ( fd > -1 ) close(fd);
}

//----------------------------------------------------------------------
// I/O Methods
//----------------------------------------------------------------------

int GarminImg::open (const char *path)
{
	string::size_type idx;
	struct stat sb;

	if ( (fd= ::open(path, O_RDONLY)) == -1 ) return -1;

	if ( fstat(fd, &sb) == -1 ) {
		close(fd);
		return -1;
	}

	ptr= (byte_t *) mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ( ptr == NULL ) {
		close(fd);
		return -1;
	}
	start= bof= ptr;

	xorbyte= *bof;
	buffer.assign(1, xorbyte);

	ptr++;

	return 0;
}

int GarminImg::seek (off_t offset)
{
	seek(offset, SEEK_SET);
}

int GarminImg::seek (off_t offset, int whence)
{
	if ( whence == SEEK_SET ) ptr= bof+offset;
	else if ( whence == SEEK_CUR ) ptr+= offset;
	else return -1;

	return 0;
}

off_t GarminImg::tell ()
{
	return (off_t) (ptr-bof);
}

//----------------------------------------------------------------------
// Private utility methods
//----------------------------------------------------------------------

void GarminImg::_xor (void *p, size_t len)
{
	byte_t *cp= (byte_t *) p;
	size_t i= 0;

	for (i= 0; i< len; ++i) {
		buffer[i]^= xorbyte;
		*cp^= xorbyte;
		++cp;
	}

}

uword_t GarminImg::last_seq_block (off_t stopat)
{
	uword_t blockno= get_uword();
	uword_t last= 0;
	byte_t *stopptr;

	stopptr= bof + stopat;

	while ( blockno != 0xFFFF && ptr < stopptr )
		blockno= get_uword();

	ptr-= 4;
	return get_uword();
}

//----------------------------------------------------------------------
// Garmin data type methods
//----------------------------------------------------------------------

byte_t GarminImg::get_byte ()
{
	byte_t val;

	start= ptr;

	val= *(ptr++);
	if ( xorbyte ) val^= xorbyte;

	buffer.assign(1, val);

	return val;
}

word_t GarminImg::get_word ()
{
	word_t val;

	start= ptr;

	buffer.assign((const char *)ptr, 2);

#if WORDS_BIGENDIAN
	val= (word_t) (*ptr)|(*(ptr+1)<<8));
#else
	memcpy(&val, (const char *) ptr, 2);
#endif
	if ( xorbyte ) _xor(&val, 2);

	ptr+= 2;

	return val;
}

uword_t GarminImg::get_uword ()
{
	return (uword_t) get_word();
}

dword_t GarminImg::get_dword ()
{
	dword_t val;

	start= ptr;

	buffer.assign((const char *)ptr, 4);

#if WORDS_BIGENDIAN
	val= (dword_t) ((*ptr)|(*(ptr+1)<<8)|(*(ptr+2)<<16)|(*(ptr+3)<<24));
#else
	memcpy(&val, ptr, 4);
#endif
	if ( xorbyte ) _xor(&val, 4);
	ptr+= 4;

	return val;
}

udword_t GarminImg::get_udword ()
{
	return (udword_t) get_dword();
}

dword_t GarminImg::get_int24 ()
{
	dword_t val= (dword_t) get_uint24();

	if ( val > 0x7FFFFF ) val-= 0xFFFFFF;

	return val;
}

udword_t GarminImg::get_uint24 ()
{
	udword_t val;

	start= ptr;

	buffer.assign((const char *)ptr, 3);

	val= (udword_t) (*ptr|(*(ptr+1)<<8)|(*(ptr+2)<<16));
	if ( xorbyte ) _xor(&val, 3);

	ptr+= 3;
	return val;
}

string GarminImg::get_string ()
{
	string s;

	start= ptr;
	while ( *ptr != xorbyte ) {
		s+= (*ptr^xorbyte);
		++ptr;
	}
	s+= (*ptr^xorbyte);
	++ptr;

	buffer= s;

	return s;
}

string GarminImg::get_string (size_t length)
{
	string s;

	if ( length == 0 ) return s;

	start= ptr;
	buffer.assign((const char *)ptr, length);

	char *data= new char[length];
	if ( data == NULL ) {
		ptr+= length;
		perror("new");
		return s;
	}

	strncpy(data, (const char *) ptr, length);
	if ( xorbyte ) _xor(data, length);
	s.assign(data, length);
	delete[] data;

	ptr+= length;

	return s;
}

void GarminImg::get_cstring (void *dst, size_t length)
{
	buffer.assign((const char *)ptr, length);

	start= ptr;

	memcpy(dst, ptr, length);
	if ( xorbyte ) _xor(dst, length);
	ptr+= length;
}

string GarminImg::get_base11str (char delim)
{
	string str11;
	string::size_type idx;
	string::iterator spos;
	int term= 2;

	while ( term ) {
		byte_t ch= get_byte();

		if ( str11.empty() ) {
			if ( ch < 0x80 ) {
				// No base11 num here
				--ptr;
				return "";
			}
			sbuffer_set();
		} else sbuffer_append();

		if ( ch & 0x80 ) --term;
		str11+= base(ch & 0x7F, 11, 2);
	}

	// Remove trailing delims

	if ( (idx= str11.find_last_not_of("A")) != string::npos )
		str11.erase(idx+1);

	// Convert in-line delimiters to the char delim

	for (spos= str11.begin(); spos!= str11.end(); ++spos)
		if ( *spos == 'A' ) *spos= delim;

	sbuffer_recall();

	return str11;
}

time_t GarminImg::get_date ()
{
	struct tm stime;

	memset(&stime, 0, sizeof(stime));

	stime.tm_year= (int) get_word()-1900;
	sbuffer_set();
	stime.tm_mon= (int) get_byte();
	sbuffer_append();
	stime.tm_mday= (int) get_byte();
	sbuffer_append();
	stime.tm_hour= (int) get_byte();
	sbuffer_append();
	stime.tm_min= (int) get_byte();
	sbuffer_append();
	stime.tm_sec= (int) get_byte();
	sbuffer_append();
	stime.tm_isdst= 0;

	sbuffer_recall();

	return mktime(&stime);
}

double GarminImg::degrees (dword_t units)
{
	return ((double) units)*((double)360)/((double)(2<<23));
}

double GarminImg::degrees (dword_t units, int bits)
{
	if ( bits < 2 ) return 0;

	return ((double) units) * ((double)360)/((double)(2<<(bits-1)));
}

//----------------------------------------------------------------------
// Save buffer methods
//----------------------------------------------------------------------

string GarminImg::ibuffer (off_t *start_offset)
{
	*start_offset= (off_t) (start-bof);
	return buffer;
}

off_t GarminImg::sbuffer_tell ()
{
	return (off_t) (save-bof);
}

void GarminImg::sbuffer_set ()
{
	sbuffer= buffer;
	save= start;
}

void GarminImg::sbuffer_append ()
{
	sbuffer+= buffer;
}

void GarminImg::sbuffer_recall ()
{
	buffer= sbuffer;
	start= save;
}

void GarminImg::sbuffer_swap ()
{
	swap(buffer, sbuffer);
	swap(start, save);
}

void GarminImg::sbuffer_rtrim (size_t n)
{
	sbuffer.erase(sbuffer.size()-n);
}

void GarminImg::sbuffer_ltrim (size_t n)
{
	sbuffer.erase(0,n);
	start+= n;
}

string::size_type GarminImg::sbuffer_size ()
{
	return sbuffer.size();
}

//----------------------------------------------------------------------
// Public utility methods
//----------------------------------------------------------------------

string GarminImg::base (long num, int base, int width)
{
	long quot;
	static char digit[37]= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string val;
	val.reserve(32);

	if ( base < 2 || base > 36 || width < 1 ) return "";

	while (num) {
		val.append(1, digit[num%base]);
		num/= base;
	}

	if ( val.size() < width ) val.append(width-val.size(), '0');
	reverse(val.begin(), val.end());

	return val;
}

//----------------------------------------------------------------------
// IMG infrastructure methods
//----------------------------------------------------------------------

void GarminImg::block_size_exp (unsigned int powoftwo)
{
	bsize= (unsigned long) pow(2.0, (double) powoftwo);
}

unsigned long GarminImg::block_size ()
{
	return bsize;
}

//----------------------------------------------------------------------
// IMG FAT/file methods
//----------------------------------------------------------------------

class ImgFile *GarminImg::file_find (string name)
{
	file_list_t::iterator fpos;

	fpos= files.find(name);
	if ( fpos == files.end() ) return NULL;

	return fpos->second;
}

void GarminImg::set_fileent ()
{
	pfileent= files.begin();
}

class ImgFile *GarminImg::get_fileent ()
{
	class ImgFile *ptr;

	if ( pfileent == files.end() ) return NULL;
	ptr= pfileent->second;
	++pfileent;

	return ptr;
}

//----------------------------------------------------------------------
// Element type methods
//----------------------------------------------------------------------

string GarminImg::elem_polyline_name (uword_t type)
{
	poly_name_t::iterator ppos;

	ppos= polylines_byname.find(type);
	return ( ppos == polylines_byname.end() ) ? "unknown" : ppos->second;
}

string GarminImg::elem_polygon_name (uword_t type)
{
	poly_name_t::iterator ppos;

	ppos= polygons_byname.find(type);
	return ( ppos == polygons_byname.end() ) ? "unknown" : ppos->second;
}

string GarminImg::elem_point_name (uword_t type, uword_t subtype)
{
	point_name_t::iterator ppos;
	uword_t fulltype= (type<<8)|subtype;

	ppos= points_byname.find(fulltype);
	return ( ppos == points_byname.end() ) ? "unknown" : ppos->second;
}

string GarminImg::elem_marine_point_name (uword_t type)
{
	point_name_t::iterator ppos;

	ppos= marine_points_byname.find(type);
	return ( ppos == marine_points_byname.end() ) ? "unknown" : ppos->second;
}

