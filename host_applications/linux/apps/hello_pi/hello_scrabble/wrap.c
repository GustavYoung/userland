#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef enum {GAME_KEY_ANY, GAME_KEY_ACTION, GAME_KEY_A, GAME_KEY_B, GAME_KEY_C, GAME_KEY_D, GAME_KEY_LEFT, GAME_KEY_RIGHT, GAME_KEY_UP, GAME_KEY_DOWN, GAME_KEY_SCROLL_C, GAME_KEY_SCROLL_A } GAME_KEY_T;

void game_error(char *fmt, ...)
{
   assert(0);
}

GAME_KEY_T game_button_down( GAME_KEY_T button )
{
   return (GAME_KEY_T)0;
}

GAME_KEY_T game_button_pressed( GAME_KEY_T button )
{
   return (GAME_KEY_T)0;
}

GAME_KEY_T game_button_released( GAME_KEY_T button )
{
   return (GAME_KEY_T)0;
}

int game_get_scrollwheel(int num, int mode)
{
   return 0;
}

unsigned long game_time_microsecs (void)
{
   static int i=0;
   return i++;
}

#if 0
#include "graphics.h"
#include "vcfilesys.h"
#include "vchost_config.h"

//The number of resources we support
#define GRAPHICS_RESOURCE_HANDLE_TABLE_SIZE    (uint8_t)(64)

//defininition for the resource table
typedef struct
{
   uint8_t     in_use;

   GRAPHICS_RESOURCE_TYPE_T resource_type;

   BITMAPINFO *bitmap_info;
   
   //is this resource a videocore resource?
   uint32_t    is_videocore_resource;
   uint32_t    native_image_handle;

   //the bitmap currently in use
   HBITMAP     bitmap;
   HDC         dc;
   void *      pixel_ptr;

   //the shadow bitmap + dc
   HBITMAP     shadow_bitmap;  
   HDC         shadow_dc;

   //host side cache'd calculations
   uint8_t     pixel_size_in_bytes;
   uint32_t    pitch;

   uint32_t    vc_resource_handle;
   uint8_t     resource_handle_valid;

   uint8_t     *vmcs_image_ptr;

   uint32_t    vc_object_handle;
   uint8_t     object_handle_valid;
   
   //videocore resource ptr + size - returned via graphics_create_videocore_resource
   uint32_t    videocore_resource_ptr;

} GRAPHICS_RESOURCE_HANDLE_TABLE_T;

//the resource table
static GRAPHICS_RESOURCE_HANDLE_TABLE_T   graphics_resource_table[ GRAPHICS_RESOURCE_HANDLE_TABLE_SIZE ];

//the supported image formats - by default, assume all image formats are supported.
static uint32_t supported_image_formats = 0xFFFFFFFF;

//a flag to count the number of dispman starts that have been invoked
static uint32_t dispman_start_count = 0;

int32_t xgraphics_resource_vflip_in_place(  GRAPHICS_RESOURCE_HANDLE resource_handle )
{
   #define MAX_LINE (4*640)
   int32_t success = -1; //fail by default
   static uint8_t temp[MAX_LINE];
   if( NULL != resource_handle)
   {
      //cast the default ptr
      GRAPHICS_RESOURCE_HANDLE_TABLE_T *table_ptr = (GRAPHICS_RESOURCE_HANDLE_TABLE_T *) resource_handle;     
      int y, h = table_ptr->bitmap_info->bmiHeader.biHeight;
      int linebytes = min(table_ptr->pitch, MAX_LINE);
      for (y = 0; y < h/2 ; y++)
      {
         uint8_t *p = (uint8_t *)table_ptr->pixel_ptr + y       * table_ptr->pitch;
         uint8_t *q = (uint8_t *)table_ptr->pixel_ptr + (h-1-y) * table_ptr->pitch;
         memcpy(temp, p, linebytes);
         memcpy(p, q,    linebytes);
         memcpy(q, temp, linebytes);
      }
      success = 0;
   }

   return success;
}

int32_t graphics_get_resource_details( const GRAPHICS_RESOURCE_HANDLE resource_handle,
                                       uint32_t *width,
                                       uint32_t *height,
                                       uint32_t *pitch,
                                       GRAPHICS_RESOURCE_TYPE_T *type,
                                       void **memory_pointer,
                                       uint32_t *object_id,
                                       uint32_t *resource_id,
                                       uint32_t *native_image_handle)
{
   int32_t success = -1; //fail by default

   if( NULL != resource_handle )
   {
      //cast the default ptr
      GRAPHICS_RESOURCE_HANDLE_TABLE_T *table_ptr = (GRAPHICS_RESOURCE_HANDLE_TABLE_T *) resource_handle;   

      //flush any pending writes
      GdiFlush();

      //return the width if required
      if( NULL != width)
      {
         *width = table_ptr->bitmap_info->bmiHeader.biWidth;

         success = 0;
      }

      //return the height if required
      if( NULL != height)
      {
         *height = table_ptr->bitmap_info->bmiHeader.biHeight;

         success = 0;
      }

      //return the height if required
      if( NULL != pitch)
      {
         *pitch = table_ptr->pitch;

         success = 0;
      }

      //type?
      if( NULL != type )
      {
         *type = table_ptr->resource_type;

         success = 0;
      }

      //return the height if required
      if( NULL != memory_pointer)
      {
         if( table_ptr->is_videocore_resource )
         {
            *memory_pointer = (void *)table_ptr->videocore_resource_ptr;
         }
         else
         {
            *memory_pointer = (void *)table_ptr->pixel_ptr;
         }

         success = 0;
      }

      //return the vc_object_handle if required
      if( NULL != object_id)
      {
         *object_id = table_ptr->vc_object_handle;

         success = 0;
      }

      //return the resource_id if required
      if( NULL != resource_id)
      {
         *resource_id = table_ptr->vc_resource_handle;

         success = 0;
      }
      
      //native image handle?
      if( NULL != native_image_handle )
      {
         if( table_ptr->is_videocore_resource )
         {
            *native_image_handle = table_ptr->native_image_handle;
            
            success = 0;
         }         
      }
   }

   return success;
}

static int32_t graphics_get_new_resource_handle( GRAPHICS_RESOURCE_HANDLE *resource_handle )
{
   int32_t success = -1; //defaults to fail
   uint32_t count = 0;

   if( NULL != resource_handle)
   {
      //reset the output resource_handle
      (*resource_handle) = NULL;

      //scan the table until we find a free entry
      for(count = 0; count < GRAPHICS_RESOURCE_HANDLE_TABLE_SIZE; count++)
      {
         if( TRUE != graphics_resource_table[count].in_use )
         {
            //clear the data first
            memset( &(graphics_resource_table[count]), 0, sizeof( GRAPHICS_RESOURCE_HANDLE_TABLE_T ) );

            //mark it as used
            graphics_resource_table[count].in_use = TRUE;

            //pass back the ptr
            (*resource_handle) = (GRAPHICS_RESOURCE_HANDLE *) &(graphics_resource_table[count]);

            //success!
            success = 0;

            break;
         }
      }
   }

   return success;
}


/***********************************************************
 * Name: graphics_free_resource_handle
 * 
 * Arguments: 
 *       void
 *
 * Description: 
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
static int32_t graphics_free_resource_handle( const GRAPHICS_RESOURCE_HANDLE resource_handle )
{
   int32_t success = -1; //defaults to fail
   uint32_t count = 0;

   //scan the table until we find a free entry
   for(count = 0; count < GRAPHICS_RESOURCE_HANDLE_TABLE_SIZE; count++)
   {
      if( resource_handle == (GRAPHICS_RESOURCE_HANDLE *) &(graphics_resource_table[count]) )
      {
         //check the data is in use
         if( TRUE == graphics_resource_table[count].in_use )
         {
            //clear the data
            memset( &(graphics_resource_table[count]), 0, sizeof( GRAPHICS_RESOURCE_HANDLE_TABLE_T ) );

            //success!
            success = 0;

            break;
         }
      }
   }

   return success;
}

int32_t graphics_create_resource(const uint32_t width,
                                 const uint32_t height,
                                 const GRAPHICS_RESOURCE_TYPE_T resource_type,
                                 GRAPHICS_RESOURCE_HANDLE *resource_handle )
{
   int32_t success = -1; //fail by default
   HBITMAP bitmap_handle = NULL;
   HBITMAP original_bitmap = NULL;
   HDC dc_handle = NULL;
   void *pixel_ptr = NULL;    // pointer to pixel data allocated by CreateDIBSection

   if( NULL != resource_handle)
   {
      BITMAPINFO *bitmap_info = NULL;

      bitmap_info = (BITMAPINFO *)calloc( 1, sizeof( BITMAPINFO ) );

      if( NULL != bitmap_info)
      {
         bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
         bitmap_info->bmiHeader.biHeight = height;
         bitmap_info->bmiHeader.biWidth = width;
         bitmap_info->bmiHeader.biPlanes = 1;        
         bitmap_info->bmiHeader.biXPelsPerMeter = 0;
         bitmap_info->bmiHeader.biYPelsPerMeter = 0;
         bitmap_info->bmiHeader.biClrUsed = 0;
         bitmap_info->bmiHeader.biClrImportant = 0;
         bitmap_info->bmiHeader.biSizeImage = height * width;

         //default to success
         success = 0;

         switch( resource_type )
         {
            case GRAPHICS_RESOURCE_RGB565:
            {
               if( supported_image_formats & (1 << VC_FORMAT_RGB565) )
               {
                  bitmap_info->bmiHeader.biBitCount = 16;
                  bitmap_info->bmiHeader.biCompression = BI_RGB;
               }
               else
               {
                  //this image format is not supported by VMCS.
                  //rebuild the VMCS distribution with USE_RGB565 enabled
                  success = -1;
                  vc_assert( 0 );
               }
            }
            break;

            case GRAPHICS_RESOURCE_RGBA32:
            {
               if( supported_image_formats & (1 << VC_FORMAT_RGBA32) )
               {
                  bitmap_info->bmiHeader.biBitCount = 32;
                  bitmap_info->bmiHeader.biCompression = BI_RGB;
               }
               else
               {
                  //this image format is not supported by VMCS.
                  //rebuild the VMCS distribution with USE_RGBA32 enabled
                  success = -1;
                  vc_assert( 0 );
               }
            }
            break;

            default:
            {
               success = -1;
            }
            break;
         }      

         if( success >= 0 )
         {
            // Don't care about "compatible" bit
            dc_handle = CreateCompatibleDC( NULL );
            
            if( NULL != dc_handle )
            {
               //Create a DIB section (bitmap data)
               bitmap_handle = CreateDIBSection(dc_handle, bitmap_info, DIB_RGB_COLORS, &pixel_ptr, NULL, 0);

               if( (NULL != pixel_ptr) && (NULL != bitmap_handle) )
               {   
                  //put the bitmap into the context of the object
                  original_bitmap = (HBITMAP)SelectObject( dc_handle, bitmap_handle );

                  if( NULL != original_bitmap )
                  {
                     //create an compatible DC for the shadow bitmap
                     HDC shadow_dc = CreateCompatibleDC( dc_handle );

                     HBITMAP shadow_bitmap = CreateCompatibleBitmap( dc_handle, width, height );

                     if( (NULL != shadow_dc) && (NULL != shadow_bitmap) )
                     {
                        //get a new resource handle
                        success = graphics_get_new_resource_handle( resource_handle );

                        if( ( success >= 0 ) && (NULL != *resource_handle) )
                        {
                           //cast the default ptr
                           GRAPHICS_RESOURCE_HANDLE_TABLE_T *table_ptr = (GRAPHICS_RESOURCE_HANDLE_TABLE_T *) *resource_handle;

                           //store the bitmap ptr and the dc handle
                           table_ptr->bitmap = bitmap_handle;
                           table_ptr->dc = dc_handle;

                           //store the shadow bitmaps / dcs, which are used for getting a copy of the data during runtime
                           table_ptr->shadow_dc = shadow_dc;
                           table_ptr->shadow_bitmap = shadow_bitmap;
                           
                           //store the pixel ptr + width
                           table_ptr->pixel_ptr = pixel_ptr;
                           table_ptr->pixel_size_in_bytes = (bitmap_info->bmiHeader.biBitCount / 8);
                           table_ptr->pitch = table_ptr->pixel_size_in_bytes * width;

                           //store the bitmap info
                           table_ptr->bitmap_info = bitmap_info;

                           //store the bitmap type
                           table_ptr->resource_type = resource_type;

                           //success!
                           success = 0;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   vc_assert( success >= 0 );

   return success;
}

int32_t graphics_load_resource_vmcs(const char *file_name,
                                    GRAPHICS_RESOURCE_HANDLE *resource_handle )
{
   int32_t success = -1; //fail by default
   
   if( NULL != file_name)
   {
      uint32_t width = 0;
      uint32_t height = 0;
      uint32_t pitch = 0;
      GRAPHICS_RESOURCE_TYPE_T resource_type;
      void *image_data = NULL;
      uint32_t image_data_size = 0;
      
      //depending on the filename, load either a bitmap or a tga
      if( (NULL != strstr( file_name, ".bmp" )) || (NULL != strstr( file_name, ".dib" )) )
      {
         //read a bitmaps header and get back the width and height
         success = graphics_common_read_bitmap_header_via_vmcs(file_name, &width, &height,
                                                               &pitch, &resource_type, &image_data_size );
      }
      else if( NULL != strstr( file_name, ".tga" ) )
      {
         //read a tga header and get back the width and height
         success = graphics_common_read_tga_header_via_vmcs(file_name, &width, &height,
                                                            &pitch, &resource_type, &image_data_size );         
      }
      
      if( success >= 0 )
      {
         //create a buffer to read the bitmap into
         image_data = malloc( image_data_size );
         
         if( (NULL != strstr( file_name, ".bmp" )) || (NULL != strstr( file_name, ".dib" )) )
         {         
            //read a bitmap into a buffer
            success = graphics_common_read_bitmap_file_from_vmcs( file_name, image_data, image_data_size );
         }
         else if( NULL != strstr( file_name, ".tga" ) )
         {
            //read a bitmap into a buffer
            success = graphics_common_read_tga_file_from_vmcs( file_name, image_data, image_data_size );
         }
      }

      if( ( success >= 0 ) && (NULL != image_data) )
      {
         //if the bitmap is 888, convert to RGBA32
         if( GRAPHICS_RESOURCE_RGB888 == resource_type )
         {
            //create an RGBA32 resource, and copy the 888 buffer to it
            success = graphics_create_resource( width, height, GRAPHICS_RESOURCE_RGBA32, resource_handle );

            //check it worked
            if( ( success >= 0 ) && (NULL != *resource_handle) )
            {
               GRAPHICS_RESOURCE_HANDLE_TABLE_T *table_ptr = (GRAPHICS_RESOURCE_HANDLE_TABLE_T *) *resource_handle;
               uint32_t pixel_size_in_bytes = (table_ptr->bitmap_info->bmiHeader.biBitCount / 8 );
               
               success = graphics_rgb888_to_rgba32_unpack(  image_data,
                                                            pitch,
                                                            width,
                                                            height,
                                                            table_ptr->pixel_ptr,
                                                            width * pixel_size_in_bytes,
                                                            0xFF );
            }
         }
         else
         {
            //create a new resource to copy this data into
            success = graphics_create_resource( width, height, resource_type, resource_handle );

            //check it worked
            if( ( success >= 0 ) && (NULL != *resource_handle) )
            {
               GRAPHICS_RESOURCE_HANDLE_TABLE_T *table_ptr = (GRAPHICS_RESOURCE_HANDLE_TABLE_T *) *resource_handle;
               uint32_t height_count = 0;
               uint32_t pixel_size_in_bytes = (table_ptr->bitmap_info->bmiHeader.biBitCount / 8 );

               //now, we need to copy the data over from our buffer (which has a pitch), to our PC based buffer
               for( height_count = 0; height_count < height; height_count++)
               {
                  uint8_t *src_row_ptr = ((uint8_t *)image_data + (height_count * pitch));
                  uint8_t *dest_row_ptr = ((uint8_t *)table_ptr->pixel_ptr + height_count);

                  memcpy( dest_row_ptr, src_row_ptr, width * pixel_size_in_bytes );
               }
            }
         }

         //free the data from the bitmap read
         free( image_data );
      }
   }

   return success;
}

/***********************************************************
 * Name: graphics_read_bitmap_file
 * 
 * Arguments: 
 *       void
 *
 * Description: Reads in the bitmap header
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
//the bitmap header size
#define BITMAP_HEADERINFOSIZE ((5 * sizeof( uint16_t )) + (11 * sizeof( uint32_t )))

//The TGA fileformat header
typedef struct
{
    uint8_t    identsize;          // size of ID field that follows 18 byte header (0 usually)
    uint8_t    colourmaptype;      // type of colour map 0=none, 1=has palette
    uint8_t    imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    uint16_t   colourmapstart;     // first colour map entry in palette
    uint16_t   colourmaplength;    // number of colours in palette
    uint8_t    colourmapbits;      // number of bits per palette entry 15,16,24,32

    uint16_t   xstart;             // image x origin
    uint16_t   ystart;             // image y origin
    uint16_t   width;              // image width in pixels
    uint16_t   height;             // image height in pixels
    uint8_t    bits;               // image bits per pixel 8,16,24,32
    uint8_t    descriptor;         // image descriptor bits (vh flip bits)    
    
    // pixel data follows header
} TGA_HEADER_T;

#define TGA_HEADERINFOSIZE ((6 * sizeof( uint8_t )) + (6 * sizeof( uint16_t )))

#define G8  buffer[i++]
#define G16 ((buffer[i+1] << 8) | buffer[i]); i+=2
#define G32 ((buffer[i+3] << 24) | (buffer[i+2] << 16) | (buffer[i+1] << 8) | buffer[i]); i+=4

static int32_t graphics_read_bitmap_header_info_from_file( int file,
                                                           BITMAPFILEHEADER *header,
                                                           BITMAPINFOHEADER *info )
{
   int32_t success = -1; //fail by default

   if( (0 != file) && (NULL != header) && (NULL != info) )
   {
      int data_read = 0;
      uint8_t buffer[BITMAP_HEADERINFOSIZE ];
      int i=0;

      //read the data into the buffer
      data_read += vc_filesys_read( file, (void *)buffer, BITMAP_HEADERINFOSIZE);


      if( data_read == BITMAP_HEADERINFOSIZE )
      {
         //it was successful!
         success = 0;

         header->bfType = G16;
         header->bfSize = G32;
         header->bfReserved1 = G16;
         header->bfReserved2 = G16;
         header->bfOffBits = G32;

         info->biSize = G32;
         info->biWidth = G32;
         info->biHeight = G32;
         info->biPlanes = G16;
         info->biBitCount = G16;
         info->biCompression = G32;
         info->biSizeImage = G32;
         info->biXPelsPerMeter = G32;
         info->biYPelsPerMeter = G32;
         info->biClrUsed = G32;
         info->biClrImportant = G32;
      }
   }

   return success;
}

/***********************************************************
 * Name: graphics_read_bitmap_file
 * 
 * Arguments: 
 *       void
 *
 * Description: Reads in the bitmap header
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
static int32_t graphics_read_tga_header_info_from_file(  int file,
                                                         TGA_HEADER_T *header )
{
   int32_t success = -1; //fail by default

   if( (0 != file) && (NULL != header) )
   {
      int data_read = 0;
      uint8_t buffer[TGA_HEADERINFOSIZE ];
      int i=0;

      //read the data into the buffer
      data_read += vc_filesys_read( file, (void *)buffer, TGA_HEADERINFOSIZE);

      if( data_read == TGA_HEADERINFOSIZE )
      {
         //it was successful!
         success = 0;

         header->identsize = G8;
         header->colourmaptype = G8;
         header->imagetype = G8;
         
         header->colourmapstart = G16;
         header->colourmaplength = G16;
         header->colourmapbits = G8;
         
         header->xstart = G16;
         header->ystart = G16;
         header->width = G16;
         header->height = G16;
         
         header->bits = G8;
         header->descriptor = G8;           
      }
   }

   return success;
}

/***********************************************************
 * Name: graphics_common_read_bitmap_header_via_vmcs
 * 
 * Arguments: 
 *       const char *file_name,
         uint32_t *width,
         uint32_t *height,
         uint32_t *pitch,
         GRAPHICS_RESOURCE_TYPE_T *resource_type
 *
 * Description: Reads a bitmap header and extracts the image size
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
int32_t graphics_common_read_bitmap_header_via_vmcs(  const char *file_name,
                                                      uint32_t *width,
                                                      uint32_t *height,
                                                      uint32_t *pitch,
                                                      GRAPHICS_RESOURCE_TYPE_T *resource_type,
                                                      uint32_t *image_data_size )
{
   int32_t success = -1; //fail by default

   //check the parameters
   if( (NULL != width) && (NULL != height) && (NULL != pitch) )
   {
      int file = 0;

      //open the file
      file = vc_filesys_open( file_name, VC_O_RDONLY );

      if( -1 != file )
      {
         int data_read = 0;
         BITMAPFILEHEADER bitmap_header;
         BITMAPINFOHEADER bitmap_info;

         //read the bitmap header and info
         success = graphics_read_bitmap_header_info_from_file( file, &bitmap_header, &bitmap_info );

         if( success >= 0)
         {
            //set to fail
            success = -1;

            //check the file size matches the file size field AND the special chars are set
            if( 0x4d42 == bitmap_header.bfType ) // 0x42 = "B" 0x4d = "M" 
            {
               success = 0;

               //store the dimenstions
               *width = bitmap_info.biWidth;
               *height = bitmap_info.biHeight;
               
               //worst the resource type out
               if( 16 == bitmap_info.biBitCount )
               {
                  *resource_type = GRAPHICS_RESOURCE_RGB565;
               }
               else if( 24 == bitmap_info.biBitCount )
               {
                  *resource_type = GRAPHICS_RESOURCE_RGB888;
               }
               else if( 32 == bitmap_info.biBitCount )
               {
                  *resource_type = GRAPHICS_RESOURCE_RGBA32;
               }
               else
               {
                  success = -1;
               }
               
               if( success >= 0 )
               {
                  //calculate the pitch
                  *pitch = ((*width) * (bitmap_info.biBitCount / 8 ));
                  *pitch = ((((*pitch) + 3) / 4) * 4);
                  
                  //get the required buffer size
                  *image_data_size = ((*pitch) * (*height));
               }
            }
         }

         //close the file
         vc_filesys_close( file );
      }
   }

   return success;
}


/***********************************************************
 * Name: graphics_common_read_tga_header_via_vmcs
 * 
 * Arguments: 
 *       const char *file_name,
         uint32_t *width,
         uint32_t *height,
         uint32_t *pitch,
         GRAPHICS_RESOURCE_TYPE_T *resource_type
 *
 * Description: Reads a tga header and extracts the image size
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
int32_t graphics_common_read_tga_header_via_vmcs(  const char *file_name,
                                                   uint32_t *width,
                                                   uint32_t *height,
                                                   uint32_t *pitch,
                                                   GRAPHICS_RESOURCE_TYPE_T *resource_type,
                                                   uint32_t *image_data_size )
{
   int32_t success = -1; //fail by default

   //check the parameters
   if( (NULL != width) && (NULL != height) && (NULL != pitch) )
   {
      int file = 0;

      //open the file
      file = vc_filesys_open( file_name, VC_O_RDONLY );

      if( -1 != file )
      {
         int data_read = 0;
         TGA_HEADER_T tga_header;

         //read the bitmap header and info
         success = graphics_read_tga_header_info_from_file( file, &tga_header );

         if( success >= 0)
         {
            //set to fail
            success = -1;

            //check that the imagetype is ok and there is no colour map 
            if( (tga_header.imagetype == 2) && (tga_header.colourmaptype == 0) )
            {
               success = 0;

               //store the dimenstions
               *width = tga_header.width;
               *height = tga_header.height;
               
               //worst the resource type out
               if( 24 == tga_header.bits )
               {
                  *resource_type = GRAPHICS_RESOURCE_RGB888;
               }
               else if( 32 == tga_header.bits )
               {
                  *resource_type = GRAPHICS_RESOURCE_RGBA32;
               }
               else
               {
                  success = -1;
               }
               
               if( success >= 0 )
               {
                  //calculate the pitch
                  *pitch = ((*width) * (tga_header.bits / 8 ));
                  
                  //get the required buffer size
                  *image_data_size = ((*pitch) * (*height));
               }
            }
         }

         //close the file
         vc_filesys_close( file );
      }
   }

   return success;
}

/***********************************************************
 * Name: graphics_common_read_bitmap_file_from_vmcs
 * 
 * Arguments: 
 *       const char *file_name,
         void *image_data,
         const uint32_t image_data_size
 *
 * Description: Reads a bitmap file from vmcs file system
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
int32_t graphics_common_read_bitmap_file_from_vmcs(const char *file_name,
                                                   void *image_data,
                                                   const uint32_t image_data_size )
{
   int32_t success = -1; //fail by default

   //check the parameters
   if( ( NULL != image_data) && (0 != image_data_size) )
   {
      int file = 0;

      //open the file
      file = vc_filesys_open( file_name, VC_O_RDONLY );

      if( -1 != file )
      {
         int file_size = 0;

         //seek to the end of the file and get the size
         file_size = vc_filesys_lseek(file,0,VC_FILESYS_SEEK_END);

         if( file_size > 0 )
         {
            int data_read = 0;
            BITMAPFILEHEADER bitmap_header;
            BITMAPINFOHEADER bitmap_info;

            //seek to the start of the file
            vc_filesys_lseek( file,0,VC_FILESYS_SEEK_SET);

            //read the bitmap header and info
            success = graphics_read_bitmap_header_info_from_file( file, &bitmap_header, &bitmap_info );

            if( success >= 0)
            {
               //set to fail
               success = -1;

               //check the file size matches the file size field AND the special chars are set
               if( ( bitmap_header.bfSize == file_size ) && (0x4d42 == bitmap_header.bfType) ) // 0x42 = "B" 0x4d = "M" 
               {
                  //check that the required buffer sizeis not less than the rest of the file
                  if( image_data_size >= (file_size - bitmap_header.bfOffBits) )
                  {
                     //seek to the start of the data
                     vc_filesys_lseek( file, bitmap_header.bfOffBits, VC_FILESYS_SEEK_SET);
                     
                     //read the data into the buffer
                     data_read = vc_filesys_read( file, image_data, file_size - bitmap_header.bfOffBits );
                     
                     //check the data read ok
                     if( data_read == (file_size - bitmap_header.bfOffBits) )
                     {
                        success = 0;
                     }
                  }
                  else
                  {
                     assert( 0 );
                  }
               }
            }
         }

         //close the file
         vc_filesys_close( file );
      }
   }

   return success;
}


/***********************************************************
 * Name: graphics_common_read_tga_file_from_vmcs
 * 
 * Arguments: 
 *       const char *file_name,
         void *image_data,
         const uint32_t image_data_size
 *
 * Description: Reads a tga file from vmcs file system
 *
 * Returns: int32_t:
 *               >=0 if it succeeded
 *
 ***********************************************************/
int32_t graphics_common_read_tga_file_from_vmcs(const char *file_name,
                                                void *image_data,
                                                const uint32_t image_data_size )
{
   int32_t success = -1; //fail by default

   //check the parameters
   if( ( NULL != image_data) && (0 != image_data_size) )
   {
      int file = 0;

      //open the file
      file = vc_filesys_open( file_name, VC_O_RDONLY );

      if( -1 != file )
      {
         int file_size = 0;

         TGA_HEADER_T tga_header;
         int n;
         uint8_t temp;
         uint8_t *img_data = image_data;

         //read the bitmap header and info
         success = graphics_read_tga_header_info_from_file( file, &tga_header );

         //seek to the start of the file
         vc_filesys_lseek( file, TGA_HEADERINFOSIZE, VC_FILESYS_SEEK_SET);

         //read the bitmap header and info
         if( image_data_size == vc_filesys_read( file, image_data, image_data_size ) )
         {
            success = 0;
         }
         
         // Note: TGA files seem to have bytes in order RBG
         // now swap bytes according to wether we have alpha channel or not          
         if(tga_header.bits == 24) {
            for(n = 0; n < tga_header.width*tga_header.height; n++) {
               temp = img_data[n*3+1];
               img_data[n*3+1] = img_data[n*3+2];
               img_data[n*3+2] = temp;
            }
            //*data_type = TGA_TYPE_RGB;
         } else { // 32bpp
            for(n = 0; n < tga_header.width*tga_header.height; n++) {
               temp = img_data[n*4+1];
               img_data[n*4+1] = img_data[n*4+2];
               img_data[n*4+2] = temp;
            }
            //*data_type = TGA_TYPE_RGBA;
         }
         // check the flip bit
         if ((tga_header.descriptor & 0x20)==0)
         {
            #define MAX_LINE (4*640)
            static uint8_t temp[MAX_LINE];
            int y, h = tga_header.height;
            int pitch = tga_header.width * tga_header.bits >> 3;
            int linebytes = min(pitch, MAX_LINE);
            for (y = 0; y < h/2 ; y++)
            {
               uint8_t *p = img_data + y       * pitch;
               uint8_t *q = img_data + (h-1-y) * pitch;
               memcpy(temp, p, linebytes);
               memcpy(p, q,    linebytes);
               memcpy(q, temp, linebytes);
            }
         }

         //close the file
         vc_filesys_close( file );
      }
   }

   return success;
}

static int graphics_rgb888_to_rgba32_unpack( void *source_data,
                                             const uint32_t src_pitch,
                                             const uint32_t width,
                                             const uint32_t height,
                                             void *dest_data,
                                             const uint32_t dest_pitch,
                                             const uint8_t alpha )
{
   int success = -1; //failure by default
   int32_t height_count = 0;
   int32_t width_count = 0;

   //param check
   if( (NULL != source_data) && (NULL != dest_data) )
   {
      //success!
      success = 0;

      //do the unpacking!
      for( height_count = (height - 1); height_count >= 0; height_count-- )
      {
         //work out the correct rows to work on
         uint8_t *height_src_ptr = ((uint8_t *)source_data + (height_count * src_pitch));
         uint8_t *height_dest_ptr = ((uint8_t *)dest_data + (height_count * dest_pitch));
         register uint32_t rgba32_value = 0;

         for( width_count = (width - 1); width_count >= 0; width_count--)
         {
            rgba32_value = GRAPHICS_RGB888_TO_RGBA32( (height_src_ptr + (width_count * 3))[2],
                                                      (height_src_ptr + (width_count * 3))[1],
                                                      (height_src_ptr + (width_count * 3))[0],
                                                      alpha);

            //copy over the RGBA value
            *((uint32_t *)(height_dest_ptr + (width_count * 4))) = rgba32_value;
         }
      }

      //success!
      success = 0;
   }

   return success;
}
#endif
/**************************** End of file ************************************/
