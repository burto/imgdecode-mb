#include <stdio.h>
#include "garminimg.h"
#include "decoder.h"
#include "decode_img.h"

void usage ();

int main (int argc, char *argv[])
{
	class Decoder dec;
	class ImgFile *ifile;
	class GarminImg *img;

	if ( argc != 2 ) usage();

	dec.open_img(argv[1]);
	dec.set_outdir(dec.fname);
	img= dec.img;

	decode_header(&dec);

	img->set_fileent();
	while ( (ifile= img->get_fileent()) != NULL ) {
		class ImgTRE *tre;
		class ImgLBL *lbl;
		class ImgRGN *rgn;
		class ImgNET *net;
		class ImgNOD *nod;

		tre = (ImgTRE *) ifile->subfile_find("TRE");
		if(tre != NULL)
			decode_tre_header(&dec, tre);
		else
			fprintf(stderr, "TRE not found\n");

		lbl= (ImgLBL *) ifile->subfile_find("LBL");
		if(lbl != NULL)
			decode_lbl_header(&dec, lbl);
		else {
			fprintf(stderr, "LBL not found\n");
			continue;
		}

		if(tre != NULL)
			decode_tre_body();

		decode_lbl_body();

		net= (ImgNET *) ifile->subfile_find("NET");
		if ( net ) decode_net_header(&dec, net);

		nod= (ImgNOD *) ifile->subfile_find("NOD");
		if ( nod ) decode_nod_header(&dec, nod);

		rgn= (ImgRGN *) ifile->subfile_find("RGN");
		if ( rgn == NULL ) {
			fprintf(stderr, "RGN not found\n");
			continue;	
		}

		decode_rgn_header(&dec, rgn);
		decode_rgn_body();

		if ( net ) decode_net_body();
		if ( nod ) decode_nod_body();
	}
}

void usage ()
{
	fprintf(stderr, "usage: imgdecode file\n");
	exit(1);
}

