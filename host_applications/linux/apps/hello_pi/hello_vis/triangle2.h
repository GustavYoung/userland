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
// plasma attribs
   GLuint uPlasmaMatrix, uK, uTime;
   GLuint attr_vertex, uRenderMatrix, uTexture;
   float time;
   float _c[NUMCONSTS];
   float _ct[NUMCONSTS];
   float _cv[NUMCONSTS];
} CUBE_STATE_T;


void plasma_init_shaders(CUBE_STATE_T *state);
void plasma_init(CUBE_STATE_T *state);
void plasma_update(CUBE_STATE_T *state);
void plasma_render(CUBE_STATE_T *state);
void plasma_deinit_shaders(CUBE_STATE_T *state);


