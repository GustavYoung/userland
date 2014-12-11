/*=============================================================================
Copyright (c) 2005 Broadcom Ltd. All rights reserved.

Project  :  VideoCore III 3D Renderer
Module   :  Reference Renderer
File     :  $RCSfile: graphics3.c,v $
Revision :  $Revision: 1.2 $

FILE DESCRIPTION
Top level. Main GLUT Code for drawing windows etc.

MODIFICATION DETAILS
$Log: graphics3.c,v $
Revision 1.2  2007/06/27 22:21:51  dc4
Glut now handles starting core 1

Revision 1.1  2007/06/27 22:02:37  dc4
Updated to use openglite

=============================================================================*/
#ifdef SIMPENROSE
#define USE_VBO
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define INIT_X (0)
#define INIT_Y (0)
#define INIT_Z (-derate-110)

int want_lighting = 1;
float derate = 00.0f;

float myw;
float myh;

void scrabble_do_update_idle(void);
void scrabble_do_init(void);
void scrabble_display(void);

// GLOBALS
int frames;
double xPos,yPos,zPos, xRot,yRot,zRot;
double xLightPos, yLightPos, zLightPos;

// the current model we are using
typedef struct {
   int numtriangles;
   int numvertexes;
   void *texture_data;
   int texture_type, texture_width, texture_height;
   GLfloat *st;
   GLshort *xyzshort;
   float *xyz;
   GLfloat *normal;
   GLuint texture_id;
} Model_t;

static Model_t *model;
Model_t *modelcube;

void setup_proj(void);

static void init_glstate(void)
{
   glClearColor(0.0,0.0,0.0,1.0);
   glShadeModel(GL_SMOOTH);
   //glDisable(GL_DEPTH_TEST);
   glEnable(GL_DEPTH_TEST);
   glClearDepthf(1.0);
   glDepthFunc(GL_LESS);

   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   //glFrontFace(GL_CW);
   glFrontFace(GL_CCW);

   glEnableClientState(GL_VERTEX_ARRAY);
//   glEnableClientState(GL_NORMAL_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

#if 1
{
float noAmbient[] = {1.0f, 1.0f, 1.0f, 1.0f};
//float whiteDiffuse[] = {0.1f, 0.1f, 0.1f, 1.0f};
/*
* Directional light source (w = 0)
* The light source is at an infinite distance,
* all the ray are parallel and have the direction (x, y, z).
*/

glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
//glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
}  
   glEnable(GL_LIGHT0);
   glEnable(GL_RESCALE_NORMAL);
#endif
}

static void change_texture(Model_t *model, float top_left_x, float top_left_y, float size_x, float size_y)
{
   int i=0;

   // bottom right
   model->st[i++] = top_left_x + size_x;
   model->st[i++] = 1.0f-top_left_y;
   // top right
   model->st[i++] = top_left_x + size_x;
   model->st[i++] = 1.0f-(top_left_y + size_y);
   // bottom left
   model->st[i++] = top_left_x;
   model->st[i++] = 1.0f-top_left_y;
   // top left
   model->st[i++] = top_left_x;
   model->st[i++] = 1.0f-(top_left_y + size_y);
   assert(i==2*model->numvertexes);
}

static Model_t *LoadMd3Model(const char *filename, const char *name) {
   Model_t *model = (Model_t*)malloc(sizeof *model);
   int i;
   
   model->numtriangles = 2;
   model->numvertexes = 4;
   model->st = (GLfloat*)malloc(model->numvertexes * 2 * sizeof *model->st);
   model->xyz = (float*)malloc(model->numvertexes * 4 * sizeof *model->xyz);
   //model->indexes = malloc(3 * model->numtriangles * sizeof *model->indexes);
   model->normal  = (GLfloat*)malloc(model->numvertexes * 4 * sizeof *model->normal);

   #if 0
   i = 0; // indexes
   model->indexes[i++] = 0, model->indexes[i++] = 1, model->indexes[i++] = 2;
   model->indexes[i++] = 1, model->indexes[i++] = 3, model->indexes[i++] = 2;
   assert(i==3 * model->numtriangles);
   #endif
   i = 0; // vertexes
   model->xyz[i++] = -10, model->xyz[i++] =  10, model->xyz[i++] = 0; model->xyz[i++] = 1;
   model->xyz[i++] =  10, model->xyz[i++] =  10, model->xyz[i++] = 0; model->xyz[i++] = 1;
   model->xyz[i++] = -10, model->xyz[i++] = -10, model->xyz[i++] = 0; model->xyz[i++] = 1;
   model->xyz[i++] =  10, model->xyz[i++] = -10, model->xyz[i++] = 0; model->xyz[i++] = 1;
   assert(i==model->numvertexes * 4);

   i = 0; // normals
   model->normal[i++] =   0, model->normal[i++] =   0, model->normal[i++] = 1; model->normal[i++] = 1;
   model->normal[i++] =   0, model->normal[i++] =   0, model->normal[i++] = 1; model->normal[i++] = 1;
   model->normal[i++] =   0, model->normal[i++] =   0, model->normal[i++] = 1; model->normal[i++] = 1;
   model->normal[i++] =   0, model->normal[i++] =   0, model->normal[i++] = 1; model->normal[i++] = 1;
   assert(i==model->numvertexes * 4);

   return model;
}

#include "tga.h"
static int LoadMd3ModelTextures(Model_t *model, const char *texture_dir) {
   int width, height, tga_type;
   const char *filename = "skin.tga";
   // load (NB TGA data starts bottom left, flip y as md3 s,t coords assume top left)
   model->texture_data = LoadTga(filename, &tga_type, &width, &height, 0);
   if (!model->texture_data) {
      assert(0);
      return 0; // error loading file            
   }

   
   model->texture_width = width;
   model->texture_height = height;

   if (tga_type == TGA_TYPE_RGB)
   {
      printf("RGB\n");
      model->texture_type = GL_RGB;
   }
   else
   {
      printf("RGBA\n"); 
      model->texture_type = GL_RGBA;     
   } 

   return 0;
}

static unsigned int notsorand()
{
   static unsigned int foo = 0;
   foo += 0x23456789;
   return foo;
}

// Number of cubes along each side
#define NUMCUBES 50
// Overall width of the structure
#define EXTENT 200.0f

static Model_t *MakeCubeModel() {
   Model_t *model = (Model_t*)malloc(sizeof *model);
   int i=0;
   int i2=0;
   int x,y;
   int v=0;  
   int z;
   int taken[NUMCUBES]; 
   int t;
   int repeatnum=10;
   float *xyz;  // Need higher precision when generating coordinates
   
   model->numtriangles = 8*NUMCUBES*NUMCUBES*2;
   model->numvertexes = 10*NUMCUBES*NUMCUBES*2;
   model->st = (GLfloat*)malloc(model->numvertexes * 2 * sizeof *model->st);
   model->xyz = (float*)malloc(model->numvertexes * 4 * sizeof (float));
   model->xyzshort = (GLshort*)malloc(model->numvertexes * 4 * sizeof *model->xyzshort);
   //model->indexes = malloc(3 * model->numtriangles * sizeof *model->indexes);
   
   for(y=0;y<NUMCUBES;y++)
   {
      for(x=0;x<NUMCUBES;x++)
         taken[x]=0;
        
      for(x=0;x<NUMCUBES;x++)
      {
         float fx = x*EXTENT/NUMCUBES - EXTENT*(NUMCUBES-1)/(2*NUMCUBES); // This gives a fx value with average value 0
         float fy = y*EXTENT/NUMCUBES - EXTENT*(NUMCUBES-1)/(2*NUMCUBES); // This gives a fx value with average value 0
         float fz=  0.0f;
         float e = EXTENT/(NUMCUBES*2); // This will be the additional extent in any given direction
         float tx = (float)x/NUMCUBES; 
         float ty = (float)y/NUMCUBES;
         float te = 1.0f/NUMCUBES;
         float e2;
         float s;
         
         
         
         // Now we need to pick a z value for this cube
         // It should be somewhere between 0 and NUMCUBES-1, but must not equal the z for any other cube with the same x
         do
         {
            z=(unsigned int)notsorand() % (NUMCUBES);
         //   printf("Trying %d(%d)\n",z,taken[z]);
         } while (taken[z]);
         taken[z]=1;
         fz = z*EXTENT/NUMCUBES - EXTENT*(NUMCUBES-1)/(2*NUMCUBES); // This gives a fx value with average value 0
         
         // We will display the closest surface at fx/(250-fz-e)
         // So need to equalize to make this come out to fx/250 (average position)
         // Also need the extent to match
         // Keep e for the z difference, and then calculate a new e for the x and y extent
         // need cubes or won't be able to rotate
         //
         // need s=(250-fz-e2)/250
         // e2=e*s
         // => 250.s = (250-fz-e*s)
         // => (250+e).s = 250-fz

#if(1)         
         s = (250-fz)/(250+e);
//         s = (250-fz-e)/250.0f;
         e2 = e*s;
         fx=fx*s;
         fy=fy*s;
         e2=e2*1.02;  // slightly bigger to make sure there are no gaps
         e = e2;
#else
  e2=e;
#endif         
         //printf("%d: %g,%g,%g\n",x,fx,fy,fx);
         
         model->xyz[i++] = -e2+fx, model->xyz[i++] = -e2+fy, model->xyz[i++] = e+fz;i++; // C
         model->xyz[i++] = -e2+fx, model->xyz[i++] = -e2+fy, model->xyz[i++] = e+fz;i++; // C
         model->xyz[i++] = -e2+fx, model->xyz[i++] =  e2+fy, model->xyz[i++] = e+fz;i++; // A
         model->xyz[i++] =  e2+fx, model->xyz[i++] = -e2+fy, model->xyz[i++] = e+fz;i++; // D
         model->xyz[i++] =  e2+fx, model->xyz[i++] =  e2+fy, model->xyz[i++] = e+fz;i++; // B
         model->xyz[i++] = e2+fx, model->xyz[i++] = -e2+fy, model->xyz[i++] = -e+fz;i++; // F
         model->xyz[i++] = e2+fx, model->xyz[i++] = e2+fy, model->xyz[i++] = -e+fz;i++; //E
         model->xyz[i++] = -e2+fx, model->xyz[i++] = -e2+fy, model->xyz[i++] = -e+fz;i++; //F
         model->xyz[i++] = -e2+fx, model->xyz[i++] = e2+fy, model->xyz[i++] = -e+fz;i++; //G
         // Finish with a repeated vertex to close off the triangle strip
         model->xyz[i++] = -e2+fx, model->xyz[i++] = e2+fy, model->xyz[i++] = -e+fz;i++; //G
         
         
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*0.0f; //C
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*0.0f; //C
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*1.0f; //A
         model->st[i2++] = tx+te*1.0f; model->st[i2++] = ty+te*0.0f; //D
         model->st[i2++] = tx+te*1.0f; model->st[i2++] = ty+te*1.0f; //B
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*0.0f; //F
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*1.0f; //E
         model->st[i2++] = tx+te*1.0f; model->st[i2++] = ty+te*0.0f; //D
         model->st[i2++] = tx+te*1.0f; model->st[i2++] = ty+te*1.0f; //B
         model->st[i2++] = tx+te*0.0f; model->st[i2++] = ty+te*1.0f; //E
         
         // Now repeat the last 10 with a rotation
         for(t=0;t<repeatnum;t++)
         {
            float px=model->xyz[i-repeatnum*4]-fx;
            float py=model->xyz[i+1-repeatnum*4]-fy;
            float pz=model->xyz[i+2-repeatnum*4]-fz;
            
            
            // 1,0,0 -> -1,0,0
            // 0,1,0 -> 0,0,1
            // 0,0,1 -> 0,1,0
            float nx = fx-px;
            float ny = fy+pz;
            float nz = fz+py;
            
            model->xyz[i++]=nx;
            model->xyz[i++]=ny;
            model->xyz[i++]=nz;
            i++;
            
            
            model->st[i2++]=model->st[i2-repeatnum*2];
            model->st[i2++]=model->st[i2-repeatnum*2];
            
         }
         
      }
   }
   
   assert(i==model->numvertexes * 4);
   assert(i2==model->numvertexes * 2);
   
   for(i=0;i<model->numvertexes * 4;i++)
      model->xyzshort[i] = (GLshort) model->xyz[i];
      
   
   /*i = 0; // vertexes
   model->xyz[i++] = -100, model->xyz[i++] = -100, model->xyz[i++] = 100; // C
   model->xyz[i++] = -100, model->xyz[i++] =  100, model->xyz[i++] = 100; // A
   model->xyz[i++] =  100, model->xyz[i++] = -100, model->xyz[i++] = 100; // D
   model->xyz[i++] =  100, model->xyz[i++] =  100, model->xyz[i++] = 100; // B
   model->xyz[i++] = 100, model->xyz[i++] = -100, model->xyz[i++] = -100; // F
   model->xyz[i++] = 100, model->xyz[i++] = 100, model->xyz[i++] = -100; //E
   assert(i==model->numvertexes * 3);
   
   i=0;
   model->st[i++] = 0.0f; model->st[i++] = 1.0f; //C
   model->st[i++] = 0.0f; model->st[i++] = 0.0f; //A
   model->st[i++] = 1.0f; model->st[i++] = 1.0f; //D
   model->st[i++] = 1.0f; model->st[i++] = 0.0f; //B
   model->st[i++] = 0.0f; model->st[i++] = 1.0f; //F
   model->st[i++] = 0.0f; model->st[i++] = 0.0f; //E
   assert(i==model->numvertexes * 2);*/
   
   return model;
}

static int GenerateGLModelTextures(Model_t *model)
{
   GLuint texture;
   glGenTextures(1, &texture);                  
   model->texture_id = texture;
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // texture belnds with object background
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

   glTexImage2D( GL_TEXTURE_2D,            // target
                  0,                       // level
                  model->texture_type,     // internalformat (RGB, RGBA....)
                  model->texture_width,    // width
                  model->texture_height,   // height
                  0,                       // border
                  model->texture_type,     // format (RGB, RGBA....)
                  GL_UNSIGNED_BYTE,        // type
                  model->texture_data  );  // pixel data

   glEnable(GL_TEXTURE_2D);       // Enable Texture Mapping

   return 1;
}

static int DrawMd3Model(const Model_t *model, int frame_num) {

   /* Draw Surfaces */
   glVertexPointer(3, GL_SHORT, 4*sizeof *model->xyz, model->xyz);
   glNormalPointer(GL_FLOAT, 4*sizeof *model->normal, model->normal);
   glTexCoordPointer(2, GL_FLOAT, 2*sizeof *model->st, model->st);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, model->numvertexes);
   return 0;
}

static void init(void) 
{
   init_glstate();

//   xRot = -33.0;
   xRot = 0.0f;
   yRot = 0.0;
   zRot = 0.0;

   xLightPos = 0.0f;
   xPos = INIT_X;
   yLightPos = 85.0f;
   yPos = INIT_Y;
   zLightPos = -215.0f;
   zPos = INIT_Z;

   model = LoadMd3Model("tile.md3", "vc_BOX");
   modelcube=MakeCubeModel();
   LoadMd3ModelTextures(model, "");
   LoadMd3ModelTextures(modelcube, "");
   

   GenerateGLModelTextures(model);
}

static void display_cube(float xPos, float yPos, float zPos, float xRot, float yRot, float zRot)
{
   static int initialised = 0;
   int tile = 0;
   const unsigned num_tiles_x = 6, num_tiles_y = 6;
   const unsigned tile_width = 37, tile_height = 40;
   const unsigned texture_width = 256, texture_height = 256;
   unsigned tile_x = tile_width * (tile % num_tiles_x), tile_y = tile_height * (tile / num_tiles_x);
   float top_left_x = (float)tile_x/(float)texture_width, top_left_y = (float)tile_y/(float)texture_height;
   float size_x = (float)tile_width/(float)texture_width, size_y = (float)tile_height/(float)texture_height;

   // DRAW MODEL HERE
   glPushMatrix();
   glTranslatef(0.0f, 0.0f, zPos);
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);
   glTranslatef(xPos, yPos, 0.0f);

//   glVertexPointer(3, GL_FLOAT, 4*sizeof *modelcube->xyz, modelcube->xyz);

#ifdef USE_VBO
   glBindBuffer(GL_ARRAY_BUFFER, 1);
   if (!initialised) {
      glBufferData(GL_ARRAY_BUFFER, modelcube->numvertexes * 4 * sizeof *modelcube->xyzshort, modelcube->xyzshort, GL_STATIC_DRAW);
   }
   glVertexPointer(3, GL_SHORT, 4*sizeof *modelcube->xyzshort, 0);

   glBindBuffer(GL_ARRAY_BUFFER, 2);
   if (!initialised) {
      glBufferData(GL_ARRAY_BUFFER, modelcube->numvertexes * 2 * sizeof *modelcube->st, modelcube->st, GL_STATIC_DRAW);
   }
   glTexCoordPointer(2, GL_FLOAT, 2*sizeof *modelcube->st, 0);
#else
   glVertexPointer(3, GL_SHORT, 4*sizeof *modelcube->xyzshort, modelcube->xyzshort);
   glTexCoordPointer(2, GL_FLOAT, 2*sizeof *modelcube->st, modelcube->st);
#endif

   glDrawArrays(GL_TRIANGLE_STRIP, 0, modelcube->numvertexes);
   
   glPopMatrix();

   initialised = 1;
}

static void display_tile(unsigned tile, float xPos, float yPos, float zPos, float xRot, float yRot, float zRot, int num_surfaces)
{
   const unsigned num_tiles_x = 6, num_tiles_y = 6;
   const unsigned tile_width = 37, tile_height = 40;
   const unsigned texture_width = 256, texture_height = 256;
   unsigned tile_x = tile_width * (tile % num_tiles_x), tile_y = tile_height * (tile / num_tiles_x);
   float top_left_x = (float)tile_x/(float)texture_width, top_left_y = (float)tile_y/(float)texture_height;
   float size_x = (float)tile_width/(float)texture_width, size_y = (float)tile_height/(float)texture_height;

   // DRAW MODEL HERE
   glPushMatrix();
   glTranslatef(0.0f, 0.0f, zPos);
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);
   glTranslatef(xPos, yPos, 0.0f);

   change_texture(model, top_left_x, top_left_y, size_x, size_y);

   DrawMd3Model(model, 0);
   glPopMatrix();
}

static void do_update_display(void)
{
   EGLDisplay display_id;
   EGLSurface surface;

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   //glClear (GL_COLOR_BUFFER_BIT);

   scrabble_display();
   
   assert(glGetError() == GL_NO_ERROR);
   display_id = eglGetCurrentDisplay();
   surface = eglGetCurrentSurface(EGL_DRAW);
   eglSwapBuffers(display_id,surface);
   
   frames++;
}

static void reshape (int w, int h)
{
   myw=w;
   myh=h;
   glViewport (0, 0, (GLsizei) myw, (GLsizei)myh);   
   setup_proj();
}

void setup_proj(void)
{
   float nearp = 100.0;    // 70.0;
   float farp = 500.0;   // 80.0;
   float hht;
   float hwd;
   float s =8.0f;

   //hht = (nearp+derate)*tan(45.0/2.0/180.0*M_PI);
   hht = 50.0f; // this is the width visible at the near plane (placed 110 in front of the object)
   hwd = hht*(float)myw/(float)myh;
   glMatrixMode(GL_PROJECTION);        // Change Matrix Mode to Projection
   glLoadIdentity();                   // Reset View
   glFrustumf(-hwd,hwd,-hht,hht,nearp+derate,farp+derate);
   //glOrthof(-hwd*s,hwd*s,-hht*s,hht*s,nearp,farp);
   glMatrixMode(GL_MODELVIEW);         // Return to the modelview matrix
   glLoadIdentity();  //xxx
   
}

static void button_press(unsigned char key_press, int x, int y)
{
   switch (key_press) {
#if 0
   case GLUT_KEY_UP:
      //xRot += 5;
      yLightPos += 5;
      break;

   case GLUT_KEY_DOWN:
      yLightPos -= 5;
      //xRot -= 5;
      break;

   case GLUT_KEY_LEFT:
      xLightPos -= 5;
      //yRot -= 5;
      break;

   case GLUT_KEY_RIGHT:
      xLightPos += 5;
      //yRot += 5;
      break;

   case GLUT_JOYSTICK_BUTTON_A:
      zLightPos -= 5;
      break;
   case GLUT_JOYSTICK_BUTTON_B:
      zLightPos += 5;
      break;
   case GLUT_JOYSTICK_BUTTON_C:
      /* zoom in */
      zPos *= 0.9;
      break;

   case GLUT_JOYSTICK_BUTTON_D:
      /* zoom out */
      zPos *= 1.1;
      break;
#endif
   default:
      printf("unknown button press %d\n", key_press);
      break;
   }
}

static void do_update_idle(void)
{
   static int count;
   scrabble_do_update_idle();
   if ((count++ & 15)==0)
//   yRot -= 10;
//   derate+=500.0f;
//   derate = 10000.0f;
//   zPos = -derate;
      do_update_display();
}

static void do_init(void)
{
   scrabble_do_init();
}

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   GLuint tex[6];
// model rotation vector and direction
   GLfloat rot_angle_x_inc;
   GLfloat rot_angle_y_inc;
   GLfloat rot_angle_z_inc;
// current model rotation angles
   GLfloat rot_angle_x;
   GLfloat rot_angle_y;
   GLfloat rot_angle_z;
// current distance from camera
   GLfloat distance;
   GLfloat distance_inc;
// pointers to texture buffers
   char *tex_buf1;
   char *tex_buf2;
   char *tex_buf3;
} CUBE_STATE_T;
static CUBE_STATE_T _state, *state=&_state;

static void init_ogl(CUBE_STATE_T *state)
{
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };
   
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
   assert(state->context!=EGL_NO_CONTEXT);

   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
   assert( success >= 0 );

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;
      
   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;        

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );
         
   dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
      
   nativewindow.element = dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );
      
   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);

   // Set background color and clear buffers
   glClearColor(0.15f, 0.25f, 0.35f, 1.0f);

   // Enable back face culling.
   glEnable(GL_CULL_FACE);

   glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char* argv[]) {
   int terminate = 0;
   bcm_host_init();

   //glutIdleFunc(do_update_idle); 
   //glutDisplayFunc(do_update_display); 
   //glutReshapeFunc(reshape);
   //glutKeyboardFunc(button_press);
   init();
do_init();
   //glutMainLoop();            

   // Start OGLES
   init_ogl(state);

   while (!terminate)
   {
      do_update_display();
   }

   return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "scrabble.h"

extern double xPos,yPos,zPos, xRot,yRot,zRot;
//extern void display_tile(unsigned tile, float xPos, float yPos, float zPos, float xRot, float yRot, float zRot, int num_surfaces);
enum {BLANK0=26, BOARD=30, GRAYTILE=BOARD, GREENTILE=BOARD, 
      DOUBLELETTER, DOUBLEWORD, TRIPLELETTER, TRIPLEWORD, STARTSTAR, 
      NUMTILES};

typedef struct {int dummy;} V3D_MATRIX4_T;
#define set_blank(blank, letter, tiles) BLANK0
#define LETTERWIDTH 20
#define TILEWIDTH 20
#define TILEHEIGHT 20
#define TILEDEPTH (TILEWIDTH/5)
#define BOUNCEMAX 64

static SCRABBLE_T static_scrabble;
void scrabble_display(void);

void scrabble_do_update_idle(void)
{
   SCRABBLE_T *scrabble = &static_scrabble;
   scrabble_logic_update(scrabble);
}
void scrabble_do_init(void)
{
   SCRABBLE_T *scrabble = &static_scrabble;
#ifdef _VIDEOCORE
  /* if (game_pak_open("cache/scrabble.pak") != GAME_OK) {
      assert(0);
   }*/
#endif
   scrabble_init(scrabble);
}

// find tile number on board
static int gui_findtile(GUI_RACK_T *guirack, int x, int y)
{
   int i;
   for (i=0; i<guirack->numletters; i++) {
      if (guirack->letters[i].onrack==0 && guirack->letters[i].tilex == x && guirack->letters[i].tiley == y)
         return i;
   }
   return -1;
}

static int rackx, racky, rackz;
void scrabble_display(void)
{
   SCRABBLE_T *scrabble = &static_scrabble;
   static int first = 1;
   int x, y;
   char text=(char)-1;
   int tile=0;
   float xtranslation = 0, ytranslation = 0, ztranslation = 0;
   GUI_T *gui = &scrabble->gui;
   BOARD_T *board = &scrabble->board;
   PLAYER_T *player = &scrabble->player[scrabble->current_player];
   GUI_RACK_T *guirack = &player->guirack;
   //float xRot=0.0f, yRot=270.0f, zRot=90.0f;
   //float xRot=-33.0f, yRot=270.0f, zRot=0.0f;
   float xPos=0.0f, yPos=0.0f, my_zPos=zPos-250.0f;
   
//      yRot -= 3/2.0f;
//      zRot -= 2/2.0f;
  //derate+=500.0f;
//   derate = 10000.0f;
   {
      static float t = 0.0;
      float t2=t*2-1;
      float t1 = (0.5 * t2 * (3 - t2 * t2 )+1.0f) * 180.0;
      t += 0.002;
      if (t >= 1.0) t -= 1.0;
      yRot = t1;
      zRot = t1;
   }

   zPos = -derate;
   
   setup_proj();
   
   //glClearDepthf(1.0);
   glDepthFunc(GL_LESS);
   
   my_zPos=zPos-250.0f;


   /*{
   float position[] = {xLightPos, yLightPos, zLightPos, 1.0f};
   glLoadIdentity();                   // Reset View
   glLightfv(GL_LIGHT0, GL_POSITION, position);
   }

   if (want_lighting)
      glEnable(GL_LIGHTING);
   else
      glDisable(GL_LIGHTING);*/

   if (0)
   for(y=0;y<BOARDSIZE;y++)
   for(x=0;x<BOARDSIZE;x++)
   {
      ytranslation = (y-MIDDLEROW)*(TILEWIDTH-1) - ((gui->viewy*(TILEWIDTH-1))>>TARGET_SCALE);
      xtranslation = (x-MIDDLEROW)*(TILEWIDTH-1) - ((gui->viewx*(TILEWIDTH-1))>>TARGET_SCALE);
      ztranslation = 0;
      display_tile(x, xPos+xtranslation, yPos+ytranslation, my_zPos+ztranslation, xRot, yRot, zRot, 1);
   }
   //first = 0;
   
   //display_tile(0, xPos, yPos, my_zPos, xRot, yRot, zRot, 1);
   
   display_cube(xPos, yPos, my_zPos, xRot, yRot, zRot);
   
#if (0)
   for(y=0;y<BOARDSIZE;y++)
   for(x=0;x<BOARDSIZE;x++)
   {
      V3D_MATRIX4_T mat;
      int letter = -1;
      float bounce = 0.0f;

      if (x==gui->tilex && y==gui->tiley)
         //bounce = gui->bounceh * TILEWIDTH >> 8;
         bounce = gui->bounceh * TILEWIDTH / 256.0f;

      ytranslation = (y-MIDDLEROW)*(TILEWIDTH-1) - ((gui->viewy*(TILEWIDTH-1))>>TARGET_SCALE);
      xtranslation = (x-MIDDLEROW)*(TILEWIDTH-1) - ((gui->viewx*(TILEWIDTH-1))>>TARGET_SCALE);
      ztranslation = 0;
      
      text = GETBOARDLETTER(board,x,y);

      // text == letter finalised on board, 0==empty, 1==A, 2==B etc
      // letter >=0 if rack letter over board tile. Now text if 1==A, 2==B etc
#if 1
      if (!text && guirack) {
         if (letter = gui_findtile(guirack, x, y), letter>=0) {
            GUI_LETTER_T *L = guirack->letters+letter;
            text = L->letter;
            bounce += TILEDEPTH*2;
         }
      }    
#endif
      if (!text && letter<0)
         ztranslation = bounce;
       
      if (x==MIDDLEROW && y==MIDDLEROW)
         tile = STARTSTAR;
      else if (get_word_mult(x,y)==3)
         tile = TRIPLEWORD;
      else if (get_word_mult(x,y)==2)
         tile = DOUBLEWORD;
      else if (get_letter_mult(x,y)==3)
         tile = TRIPLELETTER;
      else if (get_letter_mult(x,y)==2)
         tile = DOUBLELETTER;
      else 
         tile = GREENTILE;

      display_tile(tile, xPos+xtranslation, yPos+ytranslation, my_zPos+ztranslation, xRot, yRot, zRot, 1);
      
      if (text || letter>=0) {
         ztranslation = bounce+TILEDEPTH;

         if (text == 0)
            tile = BLANK0;
         else if ((text & BLANKBIT) && (text &~ BLANKBIT)<=26) {
            tile = set_blank(BLANK0, (text &~ BLANKBIT)-1, tiles);
         } else if (text<=26)
            tile = text-1;
         else assert(0);
         display_tile(tile, xPos+xtranslation, yPos+ytranslation, my_zPos+ztranslation, xRot, yRot, zRot, 1);
      }
   } 
#endif  

   //glDisable(GL_LIGHTING);
   if (0)
   {
   float position[] = {0.0f, 65.0f, -270.0f, 1.0f};
   glLoadIdentity();                   // Reset View
   glLightfv(GL_LIGHT0, GL_POSITION, position);
   }


{
   int x;
   int rotated = gui->rotated;
   int landscape = gui->landscape;
   if (!guirack) return;
   
   for(x=0; x<guirack->numletters; x++)
   {
      V3D_MATRIX4_T mat1;
      GUI_LETTER_T *L=&guirack->letters[x];
      int letter = L->letter;
      if (!L->onrack) continue;

      ytranslation = x*LETTERWIDTH-(TILES_PER_MOVE>>1)*LETTERWIDTH+racky;
      xtranslation = -80+rackx;
      ztranslation = 100+rackz;

      if (letter == 0)
         tile = BLANK0;
      else if (letter<=26)
         tile = letter-1;
      else assert(0);
      display_tile(tile, xPos+xtranslation, yPos+ytranslation, my_zPos+ztranslation, 0*xRot, 0*yRot, 1*zRot, 1);
   }   
}

   glDisable(GL_LIGHTING);
   if (0)
   {
   float position[] = {0.0f, 0.0f, -270.0f, 1.0f};
   glLoadIdentity();                   // Reset View
   glLightfv(GL_LIGHT0, GL_POSITION, position);
   }

{
   int x;
   char text;
   INFO_T *info = &scrabble->info;

   for(x=0; info->string[x]; x++)
   {
      V3D_MATRIX4_T mat;

      if (info->string[x] == 32) continue;

      xtranslation = -0*info->y;
      ytranslation = info->x+(LETTERWIDTH*x*info->width>>8);
      ztranslation = 100+0*info->z;

      text = toupper(info->string[x]);
      if (isalpha(text)) text = get_letter_index(text); else text = 0;
      if (text)
      {
         if ((text & BLANKBIT) && (text &~ BLANKBIT)<=26)
            tile = BLANK0;
         else if (text<=26)
            tile = text-1;
         //else assert(0);
         display_tile(tile, xPos+xtranslation, yPos+ytranslation, my_zPos+ztranslation, 0*xRot, 0*yRot, 1*zRot, 1);
      }
   }   
}
}
