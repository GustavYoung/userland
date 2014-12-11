/*=============================================================================
Copyright (c) 2005 Broadcom Ltd. All rights reserved.

Project  :  VideoCore III 3D Renderer
Module   :  Reference Renderer
File     :  $RCSfile: tga.c,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION
Loads/saves .tga (Truevision Targa) files
NB - 24bpp and 32bpp (alpha channel) only!

MODIFICATION DETAILS
$Log: tga.c,v $
Revision 1.1  2007/06/27 21:56:51  dc4
Initial revision

Revision 1.7  2006/08/31 12:26:42  jra
Fixed tga write fn, added texture flip for md3 models only

Revision 1.6  2006/06/08 11:53:00  jchapman
Changes to make compatible with PC and VC simulation

Revision 1.5  2006/06/02 14:14:13  avh
Integrated with VCIII simulator

Revision 1.4  2006/05/10 13:00:33  jra
fixed WriteTga modifying source data

Revision 1.3  2006/05/05 19:10:33  gck
Fixed asserts so release/optimised code works

Revision 1.2  2006/03/03 11:21:42  gck
Partial move to proper OpenGL-ES

Revision 1.1  2006/02/23 10:02:21  jra
Moved tga files to common area

Revision 1.2  2005/12/01 10:28:00  jra
Now does simple perspective correct point sampling.

Revision 1.1  2005/11/17 15:57:45  jra
First checkin


=============================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "tga.h"

// rgb = 1 => rgb, rgb = 0 => rgba
unsigned char *LoadTga(const char *filename, int *data_type, int *width, int *height, int flipy) {
   FILE *fp;
   tgaFileHeaderVC_t tga_bytes;
   tgaFileHeader_t tga_header;
   unsigned int data_size; // size of image data (in bytes) to read
   unsigned char temp;
   unsigned char *img_data;
   int bytesperrow;
   int n;

   if (!(fp = fopen(filename, "rb"))) { return 0; }
   if(!fread(&tga_bytes, sizeof(tgaFileHeaderVC_t), 1, fp)) { fclose(fp); return 0; }

   /* now fill in the structure from the bytes...*/
   tga_header.identsize = tga_bytes.identsize;          
   tga_header.colourmaptype = tga_bytes.colourmaptype;      
   tga_header.imagetype = tga_bytes.imagetype;          

   tga_header.colourmapstart = (tga_bytes.colourmapstart_hi << 8)|(tga_bytes.colourmapstart_lo);    
   tga_header.colourmaplength = (tga_bytes.colourmaplength_hi << 8)|(tga_bytes.colourmaplength_lo);    
   tga_header.colourmapbits = tga_bytes.colourmapbits;      

   tga_header.xstart = tga_bytes.xstart;             
   tga_header.ystart = tga_bytes.ystart;             
   tga_header.width = tga_bytes.width;             
   tga_header.height = tga_bytes.height;             
   tga_header.bits = tga_bytes.bits;             
   tga_header.descriptor = tga_bytes.descriptor;      

   // don't support compressed or colourmapped image types
   if(tga_header.imagetype != 2 || tga_header.colourmaptype != 0) { fclose(fp); return 0; }
   // only support 24/32 bpp
   if(tga_header.bits != 24 && tga_header.bits != 32) { fclose(fp); return 0; }
   bytesperrow = tga_header.width*(tga_header.bits>>3);
   data_size = tga_header.height*bytesperrow;

   // now read data according to data size, and format into RGB or RGBA
   // (note TGA is BGR or BGRA so we need to swap bytes)
   img_data = (unsigned char *)malloc(data_size);
   assert(img_data);

   /* correct padding that occurs with the sizeof function */
   fseek(fp,sizeof(tgaFileHeader_t)+tga_header.identsize - 2,SEEK_SET);

   
   if(flipy) {
      // Read in row reversed order if flipy nonzero
      for (n = 0; n < tga_header.height; n++) {
         size_t v = fread(&img_data[(tga_header.height-1-n)*bytesperrow],1,bytesperrow,fp);
         assert(v);
      }
   }
   else {
      for (n = 0; n < tga_header.height; n++) {
         size_t v = fread(&img_data[n*bytesperrow],1,bytesperrow,fp);
         assert(v);
      }
   }

   // now swap bytes according to wether we have alpha channel or not
   if(tga_header.bits == 24) {
      for(n = 0; n < tga_header.width*tga_header.height; n++) {
         temp = img_data[n*3];
         img_data[n*3] = img_data[n*3+2];
         img_data[n*3+2] = temp;
      }
      *data_type = TGA_TYPE_RGB;
   } else { // 32bpp
      for(n = 0; n < tga_header.width*tga_header.height; n++) {
         temp = img_data[n*4];
         img_data[n*4] = img_data[n*4+2];
         img_data[n*4+2] = temp;
      }
      *data_type = TGA_TYPE_RGBA;
   }

   *width = tga_header.width;
   *height = tga_header.height;

   fclose(fp);
   return img_data;   
}


// writes uncompressed RGB or RGBA data to file
int WriteTga(const char *filename, const unsigned char *img_data, int width, int height, int type) {
   tgaFileHeaderVC_t tga_header;
   FILE *fp;
   int n;
   size_t v;
   unsigned char r,g,b,a;
   unsigned char *w_data;

   if(type == TGA_TYPE_RGB) 
      tga_header.bits = 24; 
   else 
      tga_header.bits = 32;
   tga_header.height = height;
   tga_header.width = width;
   tga_header.imagetype = 2; // rgb type
   
   // fields we don't care about, set to zero
   tga_header.identsize  = 0;          
   tga_header.colourmaptype = 0;      
   tga_header.colourmapstart_lo = 0;
   tga_header.colourmapstart_hi = 0;
   tga_header.colourmaplength_lo = 0;
   tga_header.colourmaplength_hi = 0;
   tga_header.colourmapbits = 0;
   tga_header.xstart = 0;
   tga_header.ystart = 0;
   tga_header.descriptor = 0;   

   // open file
   if(!(fp = fopen(filename,"wb"))) { return -1; }

   // write header
   v = fwrite(&tga_header,1,sizeof(tga_header),fp);
   assert(v);

   // TGA files are in BGR or BGRA format not RGB/RGBA.
   // swap the data here...
   

   // now swap bytes according to whether we have alpha channel or not
   if(type == TGA_TYPE_RGB) {
      w_data = (unsigned char *)malloc(width*height*sizeof(unsigned char)*3);
      for(n = 0; n < width*height; n++) {
         w_data[n*3]   = img_data[n*3+2];
         w_data[n*3+1] = img_data[n*3+1];
         w_data[n*3+2] = img_data[n*3];
      }
   } else { // 32bpp
      w_data = (unsigned char *)malloc(width*height*sizeof(unsigned char)*4);
      for(n = 0; n < width*height; n++) { // form ABGR from RGBA
         r = img_data[n*4];
         g = img_data[n*4+1];
         b = img_data[n*4+2];
         a = img_data[n*4+3];
         w_data[n*4+0] = b;// blue
         w_data[n*4+1] = g;// green
         w_data[n*4+2] = r; // red
         w_data[n*4+3] = a; // alpha
      }
   }

   // write data
   if(type == TGA_TYPE_RGB) {
      // RGB
      v = fwrite(w_data,1,width*height*sizeof(unsigned char)*3,fp);
      assert(v);
   } else {
      // RGBA
      v = fwrite(w_data,1,width*height*sizeof(unsigned char)*4,fp);
      assert(v);
   }
   
   if(w_data) free(w_data);
   if(fp) fclose(fp);
   return 0;
}