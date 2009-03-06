#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <algorithm>
#include "garminimg.h"
#include "decoder.h"

using namespace std;

extern int errno;

Decoder::Decoder ()
{
	img= new GarminImg;
	fp= stdout;
	lastpos= 0;
}

Decoder::~Decoder ()
{
	close_outfile();
}

//----------------------------------------------------------------------
// General I/O methods
//----------------------------------------------------------------------

int Decoder::open_img (const char *path)
{
	string::size_type idx;

	fname= path;
	idx= fname.rfind('/');
	if ( idx != string::npos ) fname.erase(0, idx+1);

	idx= fname.rfind('.');
	if ( idx != string::npos ) fname.erase(idx);

	if ( img->open(path) ) {
		perror(path);
		return -1;
	}

	return 0;
}

int Decoder::set_outdir (string dir)
{
	struct stat sb;

	if ( stat(dir.c_str(), &sb) == -1 ) {
		if ( errno != ENOENT ) {
			perror(dir.c_str());
			return -1;
		}

		if ( mkdir(dir.c_str(), 0775) == -1 ) {
			perror(dir.c_str());
			return -1;
		}
	} else if ( ! S_ISDIR(sb.st_mode) ) {
		fprintf(stderr, "%s: not a directory\n", dir.c_str());
		return -1;
	}

	outdir= dir;

	return 0;
}

int Decoder::set_outfile (string type, string id)
{
	string path;
	char cptr[20];
	FILE *newfp;

	path= outdir;

	sprintf(cptr, "/%08x-XXXXXXXX.", img->tell());

	path+= cptr;
	path+= type;
	path+= ".";
	path+= id;

	newfp= fopen(path.c_str(), "w");
	if ( newfp == NULL ) {
		perror(path.c_str());
		return -1;
	}

	if ( fp != stdout ) close_outfile();

	fp= newfp;
	outpath= path;

	return 0;
}

void Decoder::close_outfile ()
{
	char cptr[9];

	fclose(fp);

	if ( outpath.size() ) {
		string rnpath= outpath;
		string::size_type idx;

		sprintf(cptr, "%08x", lastpos-1);

		idx= outpath.rfind("XXXXXXXX");
		rnpath.replace(idx, 8, (string) cptr);

		rename(outpath.c_str(), rnpath.c_str());
	}
}

//----------------------------------------------------------------------
// Public output methods
//----------------------------------------------------------------------

void Decoder::print (const char *fmt, ...)
{
	va_list ap;

	if ( fmt == NULL ) print(true, NULL, NULL);
	else {
		va_start(ap, fmt);
		print(true, fmt, ap);
		va_end(ap);
	}
}

void Decoder::comment (const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if ( fmt == NULL ) print(false, " ", ap);
	else print(false, fmt, ap);
	va_end(ap);

	fflush(fp);
}

void Decoder::banner (const char *str)
{
	banner((string) str);
}

void Decoder::banner (string str)
{
	string outline= "---------- " + str + " ";

	outline.append(75-outline.length(), '-');

	fprintf(fp, "%s\n", outline.c_str());
	fflush(fp);
}

void Decoder::rawprint (const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fflush(fp);
}

//----------------------------------------------------------------------
// Private output methods
//----------------------------------------------------------------------

int Decoder::print (bool usebuffer, const char *fmt, va_list ap)
{
	std::string::size_type nbytes;
	std::string::size_type idx;
	bool cont;
	off_t start;

	if (usebuffer) buffer= img->ibuffer(&start);
	else buffer.clear();

	if ( fmt == NULL ) text= "";
	else {
		vsnprintf(ctext, 2048, fmt, ap);

		text= ctext;
	}

	nbytes= buffer.size();

	if ( usebuffer ) lastpos= start+nbytes;

	idx= 0;
	while ( idx < nbytes ) {
		int i, rem, mmax;

		if ( (nbytes-idx) < 8 ) {
			mmax= (nbytes-idx);
			rem= (8-mmax)*3;
			cont= false;
		} else {
			cont= ( (nbytes-idx) == 8 ) ? false : true;
			mmax= 8;
			rem= 0;
		}

		fprintf(fp, "%08x |", start+idx);

		for (i= 0; i< mmax; ++i) {
			fprintf(fp, " %02x", (byte_t) buffer[idx++]);
		}

		if ( rem ) fprintf(fp, "%*.*s |", rem, rem, "");
		else {
			if ( cont ) fprintf(fp, " +");
			else fprintf(fp, " |");
		}

		if ( text.size() < 40 ) {
			fprintf(fp, " %s\n", text.c_str());
			text.clear();
		} else {
			fprintf(fp, " %s\n", text.substr(0, 39).c_str());
			text.erase(0, 39);
		}
	}

	while ( text.size() ) {
		if ( text.size() < 40 ) {
			fprintf(fp, "         |                         | %s\n",
				text.c_str());
			return 0;
		} else {
			fprintf(fp, "         |                         | %s\n",
				text.substr(0, 39).c_str());
			text.erase(0, 39);
		}
	}

	fflush(fp);
	return 0;
}

