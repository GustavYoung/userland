/*=============================================================================
Copyright (c) 2005 Broadcom Ltd. All rights reserved.

Project  :  VideoCore III 3D Renderer
Module   :  Reference Renderer
File     :  $RCSfile: tga.h,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION
Loads/saves .tga (Truevision Targa) files
NB - 24bpp and 32bpp (alpha channel) only!

MODIFICATION DETAILS
$Log: tga.h,v $
Revision 1.1  2007/06/27 21:56:51  dc4
Initial revision

Revision 1.5  2006/08/31 12:26:42  jra
Fixed tga write fn, added texture flip for md3 models only

Revision 1.4  2006/06/02 14:14:13  avh
Integrated with VCIII simulator

Revision 1.3  2006/05/10 13:00:33  jra
fixed WriteTga modifying source data

Revision 1.2  2006/03/15 15:02:51  jra
Tidying, implemented more ES tex fns, added multi model support + help menu

Revision 1.1  2006/02/23 10:02:21  jra
Moved tga files to common area

Revision 1.2  2005/12/01 10:28:00  jra
Now does simple perspective correct point sampling.

Revision 1.1  2005/11/17 15:57:45  jra
First checkin


=============================================================================*/

#ifndef __TGA_H_
#define __TGA_H_

#define TGA_ERROR_FILE     0
#define TGA_ERROR_FORMAT  -1
#define TGA_OK             1
#define TGA_TYPE_RGB       0x10
#define TGA_TYPE_RGBA      0x11

typedef struct {
    unsigned char  identsize;          // size of ID field that follows 18 byte header (0 usually)
    unsigned char  colourmaptype;      // type of colour map 0=none, 1=has palette
    unsigned char  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    unsigned short colourmapstart;     // first colour map entry in palette
    unsigned short colourmaplength;    // number of colours in palette
    unsigned char  colourmapbits;      // number of bits per palette entry 15,16,24,32

    unsigned short xstart;             // image x origin
    unsigned short ystart;             // image y origin
    unsigned short width;              // image width in pixels
    unsigned short height;             // image height in pixels
    unsigned char  bits;               // image bits per pixel 8,16,24,32
    unsigned char  descriptor;         // image descriptor bits (vh flip bits)    
    // pixel data follows header
} tgaFileHeader_t;

/* This is required to remove alignment issues which cause vidroCore to treat some bytes as padding*/
typedef struct {
    unsigned char  identsize;          // size of ID field that follows 18 byte header (0 usually)
    unsigned char  colourmaptype;      // type of colour map 0=none, 1=has palette
    unsigned char  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed
    unsigned char  colourmapstart_lo;     // first colour map entry in palette
    
    unsigned char  colourmapstart_hi;     // first colour map entry in palette
    unsigned char  colourmaplength_lo;    // number of colours in palette
    unsigned char  colourmaplength_hi;    // number of colours in palette
    unsigned char  colourmapbits;      // number of bits per palette entry 15,16,24,32

    unsigned short xstart;             // image x origin
    unsigned short ystart;             // image y origin

    unsigned short width;              // image width in pixels
    unsigned short height;             // image height in pixels

    unsigned char  bits;               // image bits per pixel 8,16,24,32
    unsigned char  descriptor;         // image descriptor bits (vh flip bits) 

    // pixel data follows header
} tgaFileHeaderVC_t;
// allocates memory & returns pointers on success
//unsigned char *LoadTga(const char *filename, int *data_type, int *width, int *height);
unsigned char *LoadTga(const char *filename, int *data_type, int *width, int *height, int flipy);
int WriteTga(const char *filename, const unsigned char *data, int width, int height, int type);


#endif