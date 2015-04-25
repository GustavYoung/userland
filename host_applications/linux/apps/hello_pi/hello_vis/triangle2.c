/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// OpenGL|ES 2 demo using shader to compute plasma/render sets
// Thanks to Peter de Rivas for original Python code

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"

#include "triangle2.h"

#define min(a,b) ((a)<(b)?(a):(b)) 
#define max(a,b) ((a)<(b)?(b):(a)) 

#define check() assert(glGetError() == 0)

static void showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader, sizeof log, NULL, log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader, sizeof log, NULL, log);
   printf("%d:program:\n%s\n", shader, log);
}

void plasma_init_shaders(CUBE_STATE_T *state)
{
   static const GLfloat vertex_data[] = {
        -1.0,-1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        -1.0,1.0,1.0,1.0
   };
   const GLchar *plasma_vshader_source =
              "attribute vec4 vertex;"
              "varying vec2 v_coords;"
              "void main(void) {"
              " vec4 pos = vertex;"
              " gl_Position = pos;"
              " v_coords = vertex.xy*0.5+0.5;"
              "}";
 
   //plasma
   const GLchar *plasma_fshader_source =
	"precision mediump float;"
	"const float PI=3.1415926535897932384626433832795;"
	"uniform float u_time;"
	"uniform vec2 u_k;"
	"varying vec2 v_coords;"
	"void main() {"
	"    float v = 0.0;"
	"    vec2 c = v_coords * u_k - u_k/2.0;"
	"    v += sin((c.x+u_time));"
	"    v += sin((c.y+u_time)/2.0);"
	"    v += sin((c.x+c.y+u_time)/2.0);"
	"    c += u_k/2.0 * vec2(sin(u_time/3.0), cos(u_time/2.0));"
	"    v += sin(sqrt(c.x*c.x+c.y*c.y+1.0)+u_time);"
	"    v = v/2.0;"
	"    vec3 col = vec3(1, sin(PI*v), cos(PI*v));"
	"    gl_FragColor = vec4(col*.5 + .5, 1.0);"
	"}";

   const GLchar *render_vshader_source =
              "attribute vec4 vertex;"
              "varying vec2 v_coords;"
              "void main(void) {"
              " vec4 pos = vertex;"
              " gl_Position = pos;"
              " v_coords = vertex.xy*0.5+0.5;"
              "}";
      
   // Render
   const GLchar *render_fshader_source =
	//" Uniforms"
	"uniform sampler2D uTexture;"
        "varying vec2 v_coords;"
	""
	"void main(void)"
	"{"
        //"gl_FragColor = vec4(v_coords.s, v_coords.t, 0.0, 1.0);"
	"    vec4 texture = texture2D(uTexture, v_coords);"
	"    gl_FragColor = texture;"
	"}";

        check();

        // plasma
        state->plasma_vshader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(state->plasma_vshader, 1, &plasma_vshader_source, 0);
        glCompileShader(state->plasma_vshader);
        check();

        if (state->verbose)
            showlog(state->plasma_vshader);
            
        state->plasma_fshader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(state->plasma_fshader, 1, &plasma_fshader_source, 0);
        glCompileShader(state->plasma_fshader);
        check();

        if (state->verbose)
            showlog(state->plasma_fshader);

        state->plasma_program = glCreateProgram();
        glAttachShader(state->plasma_program, state->plasma_vshader);
        glAttachShader(state->plasma_program, state->plasma_fshader);
        glLinkProgram(state->plasma_program);
        glDetachShader(state->plasma_program, state->plasma_vshader);
        glDetachShader(state->plasma_program, state->plasma_fshader);
        glDeleteShader(state->plasma_vshader);
        glDeleteShader(state->plasma_fshader);
        check();

        if (state->verbose)
            showprogramlog(state->plasma_program);

        state->attr_vertex     = glGetAttribLocation(state->plasma_program,  "vertex");
        state->uPlasmaMatrix   = glGetUniformLocation(state->plasma_program, "uPlasmaMatrix");
        state->uK              = glGetUniformLocation(state->plasma_program, "u_k");
        state->uTime           = glGetUniformLocation(state->plasma_program, "u_time");
        check();

        // render
        state->render_vshader = glCreateShader(GL_VERTEX_SHADER);
        check();
        glShaderSource(state->render_vshader, 1, &render_vshader_source, 0);
        check();
        glCompileShader(state->render_vshader);
        check();

        if (state->verbose)
            showlog(state->render_vshader);
            
        state->render_fshader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(state->render_fshader, 1, &render_fshader_source, 0);
        glCompileShader(state->render_fshader);
        check();

        if (state->verbose)
            showlog(state->render_fshader);

        state->render_program = glCreateProgram();
        glAttachShader(state->render_program, state->render_vshader);
        glAttachShader(state->render_program, state->render_fshader);
        glLinkProgram(state->render_program);
        glDetachShader(state->render_program, state->render_vshader);
        glDetachShader(state->render_program, state->render_fshader);
        glDeleteShader(state->render_vshader);
        glDeleteShader(state->render_fshader);
        check();

        if (state->verbose)
            showprogramlog(state->render_program);
            
        state->uRenderMatrix = glGetUniformLocation(state->render_program, "uRenderMatrix");
	state->uTexture      = glGetUniformLocation(state->render_program, "uTexture");
        check();
           
        // Prepare a texture image
        glGenTextures(1, &state->plasma_texture);
        check();
        glBindTexture(GL_TEXTURE_2D, state->plasma_texture);
        check();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 360, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        check();
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        check();
        // Prepare a framebuffer for rendering
        glGenFramebuffers(1, &state->plasma_fb);
        check();
        glBindFramebuffer(GL_FRAMEBUFFER, state->plasma_fb);
        check();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state->plasma_texture, 0);
        check();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        check();

        // Upload vertex data to a buffer
        glGenBuffers(1, &state->vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
        check();
}

void plasma_deinit_shaders(CUBE_STATE_T *state)
{
  glDeleteProgram(state->render_program);
  glDeleteProgram(state->plasma_program);
  check();

  glDeleteBuffers(1, &state->vertex_buffer);
  glDeleteTextures(1, &state->plasma_texture);
  check();

  glDeleteFramebuffers(1, &state->plasma_fb);
  check();
}

static void draw_plasma_to_texture(CUBE_STATE_T *state)
{
        // Draw the plasma to a texture
        glBindFramebuffer(GL_FRAMEBUFFER, state->plasma_fb);
        check();
        glClearColor( 0.0, 0.0, 0.0, 1.0 );
        glClear(GL_COLOR_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
        check();
        glUseProgram( state->plasma_program );
        check();
        glBindTexture(GL_TEXTURE_2D, state->plasma_texture);
        check();
	float alpha = 0.0f * M_PI /180.0f;
        float scaling = 0.5f;
        const GLfloat rotationMatrix[] = {
		    scaling*cosf(alpha), -scaling*sinf(alpha), 0.0f, 0.0f,
		    scaling*sinf(alpha),  scaling*cosf(alpha), 0.0f, 0.0f,
		    0.5f,                 0.5f,                1.0f, 0.0f,
                    0.0f,                 0.0f,                0.0f, 1.0f,
	};
        glUniformMatrix4fv(state->uPlasmaMatrix, 1, 0, rotationMatrix);
        check();
        glUniform2f(state->uK, 32.0f, 32.0f);
        check();
        glUniform1f(state->uTime, state->time);
        check();        
        glVertexAttribPointer(state->attr_vertex, 4, GL_FLOAT, 0, 16, 0);
        glEnableVertexAttribArray(state->attr_vertex);

        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        check();
	glDisableVertexAttribArray(state->attr_vertex);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check();
               
        glFlush();
        glFinish();
        check();
}

static void draw_triangles(CUBE_STATE_T *state)
{
        // Now render to the main frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Clear the background (not really necessary I suppose)
        glClearColor( 0.0, 1.0, 1.0, 1.0 );
        glClear(GL_COLOR_BUFFER_BIT);
        check();

        glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
        check();
        glUseProgram ( state->render_program );
        check();
        glBindTexture(GL_TEXTURE_2D, state->plasma_texture);
        check();
        glUniform1i(state->uTexture, 0); // first currently bound texture "GL_TEXTURE0"
        check();

	const GLfloat rotationMatrix[] = {
		    0.5f, 0.0f, 0.5f, 0.0f,
		    0.0f, 0.5f, 0.5f, 0.0f,
		    0.0f, 0.0f, 1.0f, 0.0f,
		    0.0f, 0.0f, 0.0f, 1.0f,
	};
        glUniformMatrix4fv(state->uRenderMatrix, 1, 0, rotationMatrix);
        check();
        glVertexAttribPointer(state->attr_vertex, 4, GL_FLOAT, 0, 16, 0);
        glEnableVertexAttribArray(state->attr_vertex);

        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        check();
	glDisableVertexAttribArray(state->attr_vertex);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//==============================================================================

static float randrange(float min, float max)
{
   return min + rand() * ((max-min) / RAND_MAX);
}

void plasma_init(CUBE_STATE_T *state)
{
  int i;
  for (i = 0; i < NUMCONSTS; ++i) {
    state->_ct[i] = randrange(0.0f, M_PI * 2.0f);
    state->_cv[i] = randrange(0.0f, 0.00005f) + 
                    0.00001f;
  }
}

void plasma_update(CUBE_STATE_T *state)
{
  int i;
	// update constants
	for (i = 0; i < NUMCONSTS; ++i) {
		state->_ct[i] += state->_cv[i];
		if (state->_ct[i] > M_PI * 2.0f)
		  state->_ct[i] -= M_PI * 2.0f;
		state->_c[i] = cosf(state->_ct[i]);
	}
  state->time += 0.01f;
  if (state->time > 12.0f + M_PI)
    state->time -= 12.0f * M_PI;
}

void plasma_render(CUBE_STATE_T *state)
{ 
  draw_plasma_to_texture(state);
  draw_triangles(state);
}

#ifdef STANDALONE

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
} EGL_STATE_T;

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

/***********************************************************
 * Name: init_ogl
 *
 * Arguments:
 *       CUBE_STATE_T *state - holds OGLES model info
 *
 * Description: Sets the display, OpenGL|ES context and screen stuff
 *
 * Returns: void
 *
 ***********************************************************/
static void init_ogl(EGL_STATE_T *state)
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
   
   static const EGLint context_attributes[] = 
   {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);
   check();

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglBindAPI(EGL_OPENGL_ES_API);
   assert(EGL_FALSE != result);
   check();

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
   assert(state->context!=EGL_NO_CONTEXT);
   check();

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
      
   check();

   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);
   check();

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);
   check();
}

int main ()
{
   int terminate = 0;
   CUBE_STATE_T _state, *state=&_state;
   EGL_STATE_T _eglstate, *eglstate=&_eglstate;

   // Clear application state
   memset( state, 0, sizeof( *state ) );
   memset( eglstate, 0, sizeof( *eglstate ) );
   state->verbose = 1;
   bcm_host_init();
   // Start OGLES
   init_ogl(eglstate);
again:
   plasma_init(state);
   plasma_init_shaders(state);

   int frames = 0;
   uint64_t ts = GetTimeStamp();
   while (!terminate)
   {
      plasma_update(state);
      plasma_render(state);
        //glFlush();
        //glFinish();
        check();
        
        eglSwapBuffers(eglstate->display, eglstate->surface);
        check();

    frames++;
    uint64_t ts2 = GetTimeStamp();
    if (ts2 - ts > 1e6)
    {
       printf("%d fps\n", frames);
       ts += 1e6;
       frames = 0;
    }
   }
   plasma_deinit_shaders(state);
   goto again;
   return 0;
}
#endif

