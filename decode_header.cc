#include <stdio.h>
#include <time.h>
#include "garminimg.h"
#include "imgtypes.h"
#include "decoder.h"
#include "decode_img.h"

int decode_header (class Decoder *dec)
{
	class GarminImg *img= dec->img;
	short e1, e2;
	byte_t byte;
	string str;
	time_t ts;
	udword_t subfile_offset;
	
	dec->set_outfile("DSKIMG", "header");
	dec->print("XOR byte 0x%02x", img->xorbyte);

	dec->print(NULL, img->get_string(9).c_str());

	dec->print("Update month %d", img->get_byte());
	dec->print("Update year %d", img->get_byte()+1900);

	dec->print(NULL, img->get_string(3).c_str());

	dec->print("Checksum 0x%02x", img->get_byte());

	dec->print("%s", img->get_string(7).c_str());

	dec->print("???", img->get_byte());
	dec->print("%u sectors?", img->get_uword());
	dec->print("%u heads?", img->get_uword());

	dec->print("???", img->get_word());
	dec->print("???", img->get_word());

	dec->print("%s", img->get_string(25).c_str());

	ts= img->get_date();
	dec->print("Created %24.24s", ctime(&ts));

	dec->print("???", img->get_byte());

	dec->print("%s", img->get_string(7).c_str());

	dec->print(NULL, img->get_byte());

	dec->print("Map description: %s", img->get_string(20).c_str());

	dec->print("%u heads?", img->get_uword());
	dec->print("%u sectors?", img->get_uword());

	e1= img->get_byte();
	dec->print("E1= %u", e1);
	e2= img->get_byte();
	dec->print("E2= %u", e2);
	img->block_size_exp(e1+e2);
	dec->comment("Block size= 2^(E1+E2)= %u", img->block_size());
	dec->print("???", img->get_uword());

	dec->print("Map desc (cntd): %s", img->get_string(31).c_str());

	dec->rawprint("*\n");

	img->seek(0x1be);

	dec->banner("Partition Table");
	dec->print("%u boot?", img->get_byte());
	dec->print("%u start-head?", img->get_byte());
	dec->print("%u start-sector?", img->get_byte());
	dec->print("%u start-cylinder?", img->get_byte());
	dec->print("%u system-type?", img->get_byte());
	dec->print("%u end-head?", img->get_byte());
	dec->print("%u end-sector?", img->get_byte());
	dec->print("%u end-cylinder?", img->get_byte());
	dec->print("%u rel-sectors?", img->get_udword());
	dec->print("%u num-sectors?", img->get_udword());
	dec->print(NULL, img->get_string(48).c_str());
	dec->print("", img->get_uword());
	dec->banner("End Partition Table");

	dec->rawprint("*\n");

	img->seek(0x400);

	dec->print("???", img->get_byte());
	dec->print("???", img->get_string(11).c_str());
	dec->print("First subfile offset: 0x%06lx", 
		subfile_offset= img->get_udword());
	dec->print("???", img->get_byte());
	dec->print("", img->get_string(15).c_str());

	dec->print("Sequence block start", img->get_uword());
	dec->rawprint("*\n");
	dec->print("Sequence block end (block %u)",
		img->last_seq_block(0x600));
	if ( img->tell() != 0x600 ) {
		dec->print("Sequence block padding", img->get_uword());
		dec->rawprint("*\n");
	}

	img->seek(0x600);

	dec->banner("Begin FAT");
	while ( img->tell() < subfile_offset ) {
		img_fat_block_t fblock;
		off_t next_block;
		byte_t valid, part;
		udword_t seq;

		next_block= img->tell()+512;
		
		valid= img->get_byte();
		if ( valid ) dec->print("Subfile block");
		else dec->print("Dummy block");

		fblock.filename= img->get_string(8);
		dec->print("Filename: %s", fblock.filename.c_str());

		fblock.extension= img->get_string(3);
		dec->print("Type: %s", fblock.extension.c_str());

		fblock.fullname= fblock.filename + "." +
			fblock.extension; 
		dec->comment("%s", fblock.fullname.c_str());

		fblock.size= (off_t) img->get_udword();

		if ( fblock.size ) dec->print("%lu bytes", fblock.size);
		else dec->print(NULL);

		dec->print(NULL, img->get_byte());
		dec->print("Part %u", part= img->get_uword());
		dec->print("???", img->get_string(13).c_str());

		seq= img->get_uword();
		dec->print("Sequence block start", seq);
		dec->rawprint("*\n");
		dec->print("Sequence block end (block %u)",
			img->last_seq_block(img->tell()+480));
	
		if ( img->tell() != next_block ) {
			dec->print("Sequence block padding", img->get_uword());
			dec->rawprint("*\n");
			img->seek(next_block);
		}
		fblock.offset= seq*img->block_size();
		if ( ! part && valid ) {
			class ImgFile *ifile;
			class ImgSubfile *subfile;

			dec->comment("Subfile at offset 0x%06lx",
				fblock.offset);
			img->fat.insert(make_pair(fblock.fullname, fblock));

			ifile= img->file_find(fblock.filename);
			if ( ifile == NULL ) {
				ifile= new ImgFile(img, fblock.filename);
				img->files.insert(make_pair(fblock.filename,
					ifile));
			}

			ifile->subfile_add(fblock.extension, fblock.offset,
				fblock.size);
		}
		dec->rawprint("\n");
	}
	dec->banner("End FAT");

	// That's it for the header block

	return 0;
}

void decode_common_header (class Decoder *dec, class ImgSubfile *sub)
{
	class ImgFile *ifile= sub->ifile;
	class GarminImg *img= dec->img;
	time_t ts;

	dec->print("Header length %u bytes", sub->hlen= img->get_uword());
	dec->print("File type %s", img->get_string(10).c_str());
	dec->print("???", img->get_byte());

	sub->locked= img->get_byte();
	dec->print((sub->locked) ? "Locked" : "Not locked");

	ts= img->get_date();
	dec->print("Created %24.24s", ctime(&ts));
}

