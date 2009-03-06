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
		class ImgSubfile *sub= ifile->subfile_find("TRE");

		if ( sub != NULL ) {
			class ImgTRE *tre= (ImgTRE *) sub;
			class ImgLBL *lbl;
			class ImgRGN *rgn;
			class ImgNET *net;
			class ImgNOD *nod;

			decode_tre_header(&dec, tre);

			lbl= (ImgLBL *) ifile->subfile_find("LBL");
			if ( lbl == NULL ) {
				fprintf(stderr, "LBL not found\n");
				return 1;
			}
			decode_lbl_header(&dec, lbl);

			decode_tre_body();
			decode_lbl_body();

			net= (ImgNET *) ifile->subfile_find("NET");
			if ( net ) decode_net_header(&dec, net);

			nod= (ImgNOD *) ifile->subfile_find("NOD");
			if ( nod ) decode_nod_header(&dec, nod);

			rgn= (ImgRGN *) ifile->subfile_find("RGN");
			if ( rgn == NULL ) {
				fprintf(stderr, "RGN not found\n");
				return 1;
			}

			decode_rgn_header(&dec, rgn);
			decode_rgn_body();

			if ( net ) decode_net_body();
			if ( nod ) decode_nod_body();
		} else {
			fprintf(stderr, "TRE not found in\n");
			return -1;
		}
	}
}

void usage ()
{
	fprintf(stderr, "usage: imgdecode file\n");
	exit(1);
}

