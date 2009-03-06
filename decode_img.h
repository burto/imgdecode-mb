#ifndef __DECODE_IMAGE__H
#define __DECODE_IMAGE__H

#include "decoder.h"

int decode_header (class Decoder *dec);

void decode_common_header (class Decoder *dec, class ImgSubfile *sub);

void decode_tre_header (class Decoder *dec, class ImgTRE *sub);
void decode_tre_body ();

void decode_lbl_header (class Decoder *dec, class ImgLBL *sub);
void decode_lbl_body ();

void decode_rgn_header (class Decoder *dec, class ImgRGN *sub);
void decode_rgn_body ();

void decode_net_header (class Decoder *dec, class ImgNET *sub);
void decode_net_body ();

void decode_nod_header (class Decoder *dec, class ImgNOD *sub);
void decode_nod_body ();

#endif
