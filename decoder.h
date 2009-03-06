#ifndef __DECODER__H
#define __DECODER__H

#include <sys/types.h>
#include <stdio.h>
#include <string>

using namespace std;

class Decoder {
	string outdir, outpath;
	string text, buffer;	// Text and data buffers
	off_t lastpos;		// Last position that was printed
	char ctext[2048];	// Plenty.
	FILE *fp;

	// Private output methods

	int print (bool usebuffer, const char *fmt, va_list ap);

public:
	class GarminImg *img;
	string fname;

	Decoder ();
	~Decoder ();

	// General I/O methods

	int open_img (const char *path);

	int set_outdir (string dir);
	int set_outfile (string type, string id);
	void close_outfile ();

	// Output methods 

	void print (const char *fmt, ...);
	void comment (const char *fmt, ...);
	void rawprint (const char *fmt, ...);
	void banner (const char *str);
	void banner (string str);
};

#endif

