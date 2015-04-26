#pragma once

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#define NUMCONSTS 9

typedef struct
{
   GLuint verbose;
   GLuint plasma_vshader;
   GLuint plasma_fshader;
   GLuint render_vshader;
   GLuint render_fshader;
   GLuint plasma_program;
   GLuint render_program;
   GLuint plasma_fb;
   GLuint plasma_texture;
   GLuint vertex_buffer;
   GLuint texture;
   void *texture_data;
// plasma attribs
   GLuint uPlasmaMatrix, uTime, uResolution, uMouse;
   GLuint uChannel0, uChannel1, uChannel2, uChannel3;
   GLuint attr_vertex, uRenderMatrix, uTexture;
   float time;
   uint64_t last_time;
   int width, height;
   float _c[NUMCONSTS];
   float _ct[NUMCONSTS];
   float _cv[NUMCONSTS];
} CUBE_STATE_T;


void warp_init_shaders(CUBE_STATE_T *state, int width, int height);
void warp_init(CUBE_STATE_T *state);
void warp_update(CUBE_STATE_T *state);
void warp_render(CUBE_STATE_T *state);
void warp_deinit_shaders(CUBE_STATE_T *state);


