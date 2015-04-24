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

// OpenGL|ES 2 demo using shader to compute particle/render sets
// Thanks to Peter de Rivas for original Python code

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#define NUM_PARTICLES 2000
#define PARTICLE_WIDTH  16
#define PARTICLE_HEIGHT 16
#define min(a,b) ((a)<(b)?(a):(b)) 
#define max(a,b) ((a)<(b)?(b):(a)) 

#define NUMCONSTS 9
#define NUMEMITTERS 30

typedef struct Particle
{
    GLfloat pos[4];
    GLfloat shade[4];
} Particle;
 
typedef struct Emitter
{
    GLfloat  x, y, z;
} Emitter;

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;

   GLuint verbose;
   GLuint vshader;
   GLuint mvshader;
   GLuint fshader;
   GLuint mshader;
   GLuint program;
   GLuint program_particle;
   GLuint tex_fb;
   GLuint tex;
   GLuint tex_particle;
   GLuint buf;
   GLuint particleBuffer;
// render attribs
   GLuint attr_vertex, unif_tex, uRotationMatrix;
// particle attribs
   GLuint aPos, aShade, uProjectionMatrix, uTexture;

   Emitter emitter[NUMEMITTERS];
   Particle particles[NUM_PARTICLES];
   void *tex_particle_data;

   float _c[NUMCONSTS];
   float _ct[NUMCONSTS];
   float _cv[NUMCONSTS];

  unsigned int numWinds;
  unsigned int numEmitters;
  unsigned int numParticles;
  unsigned int whichParticle;
  float size;
  float windSpeed;
  float emitterSpeed;
  float particleSpeed;
  float blur;
  float eVel;
} CUBE_STATE_T;

static CUBE_STATE_T _state, *state=&_state;

#define check() assert(glGetError() == 0)

static void showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
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

static void *create_particle_tex(int width, int height)
{
  int i, j;
  unsigned char *q = malloc(width * height * 4);
  if (!q)
    return NULL;
  unsigned char *p = q;
  for (j=0; j<height; j++) {
    for (i=0; i<width; i++) {
      float x = ((float)i + 0.5f) / (float)width  - 0.5f;
      float y = ((float)j + 0.5f) / (float)height - 0.5f;
      float d = 1.0f-2.0f*sqrtf(x*x + y*y);
      unsigned v = 255.0f * max(min(d, 1.0f), 0.0f);
      *p++ = 255;
      *p++ = 255;
      *p++ = 255;
      *p++ = v;
    }
  }
  return q;
}

static void init_shaders(CUBE_STATE_T *state)
{
   static const GLfloat vertex_data[] = {
        -1.0,-1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        -1.0,1.0,1.0,1.0
   };
   const GLchar *particle_vshader_source =
	//"// Attributes"
	"attribute vec4 aPos;"
        "attribute vec4 aShade;"
	""
	//"// Uniforms"
	"uniform mat4 uProjectionMatrix;"
	"varying vec4 vShade;"
        ""
	"void main(void)"
	"{"
	"    gl_Position = uProjectionMatrix * vec4(vec3(aPos)/60.0, 1.0);"
	"    gl_PointSize = 16.0;"
	"    vShade = aShade;"
	"}";
 
   //particle
   const GLchar *particle_fshader_source =
	//" Input from Vertex Shader"
	"varying vec4 vShade;"
	"" 
	//" Uniforms"
	"uniform sampler2D uTexture;"
	""
	"void main(void)"
	"{"
	"    vec4 texture = texture2D(uTexture, gl_PointCoord);"
	"    vec4 color = clamp(vShade, vec4(0.0), vec4(1.0));"
	"    gl_FragColor = texture * color;"
	"}";

   const GLchar *render_vshader_source =
              "attribute vec4 vertex;"
              "varying vec2 tcoord;"
              "uniform mat4 uRotationMatrix;"
              "void main(void) {"
              " vec4 pos = vertex;"
              " gl_Position = pos;"
              //" vec3 r = uRotationMatrix * vec3(vertex.x, vertex.y, 1.0);" why doesn't this work?
              " vec4 r = uRotationMatrix * vec4(vertex.x, vertex.y, 0.0, 1.0) + 0.5;"
	      " tcoord = r.xy;"
              "}";
      
   // Render
   const GLchar *render_fshader_source =
	"varying vec2 tcoord;"
	"uniform sampler2D tex;"
	"void main(void) {"
	"  vec4 color2;"
	"  color2 = texture2D(tex,tcoord);"
	//"  gl_FragColor = max(color2 - 0.05, 0.0);"
	"  gl_FragColor = color2 * 0.9;"
	"}";

        state->vshader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(state->vshader, 1, &render_vshader_source, 0);
        glCompileShader(state->vshader);
        check();

        if (state->verbose)
            showlog(state->vshader);
            
        state->mvshader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(state->mvshader, 1, &particle_vshader_source, 0);
        glCompileShader(state->mvshader);
        check();

        if (state->verbose)
            showlog(state->mvshader);
            
        state->fshader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(state->fshader, 1, &render_fshader_source, 0);
        glCompileShader(state->fshader);
        check();

        if (state->verbose)
            showlog(state->fshader);

        state->mshader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(state->mshader, 1, &particle_fshader_source, 0);
        glCompileShader(state->mshader);
        check();

        if (state->verbose)
            showlog(state->mshader);

        // render
        state->program = glCreateProgram();
        glAttachShader(state->program, state->vshader);
        glAttachShader(state->program, state->fshader);
        glLinkProgram(state->program);
        check();

        if (state->verbose)
            showprogramlog(state->program);
            
        state->attr_vertex = glGetAttribLocation(state->program, "vertex");
        state->unif_tex    = glGetUniformLocation(state->program, "tex");       
        state->uRotationMatrix   = glGetUniformLocation(state->program, "uRotationMatrix");

        // particle
        state->program_particle = glCreateProgram();
        glAttachShader(state->program_particle, state->mvshader);
        glAttachShader(state->program_particle, state->mshader);
        glLinkProgram(state->program_particle);
        check();

        if (state->verbose)
            showprogramlog(state->program_particle);
            
        state->aPos              = glGetAttribLocation(state->program_particle, "aPos");
        state->uProjectionMatrix = glGetUniformLocation(state->program_particle, "uProjectionMatrix");
	state->aShade            = glGetAttribLocation(state->program_particle, "aShade");
	state->uTexture          = glGetUniformLocation(state->program_particle, "uTexture");
        check();
           
        glGenBuffers(1, &state->buf);
        glGenBuffers(1, &state->particleBuffer);

        check();

        // Prepare a texture image
        glGenTextures(1, &state->tex_particle);
        check();
        glBindTexture(GL_TEXTURE_2D, state->tex_particle);
        check();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	state->tex_particle_data = create_particle_tex(PARTICLE_WIDTH, PARTICLE_HEIGHT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PARTICLE_WIDTH, PARTICLE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->tex_particle_data);
        check();


        // Prepare a texture image
        glGenTextures(1, &state->tex);
        check();
        glBindTexture(GL_TEXTURE_2D, state->tex);
        check();
        // glActiveTexture(0)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state->screen_width, state->screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        check();
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        check();
        // Prepare a framebuffer for rendering
        glGenFramebuffers(1,&state->tex_fb);
        check();
        glBindFramebuffer(GL_FRAMEBUFFER,state->tex_fb);
        check();
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,state->tex,0);
        check();
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        check();
        // Prepare viewport
        glViewport ( 0, 0, state->screen_width, state->screen_height );
        check();
        
        // Upload vertex data to a buffer
        glBindBuffer(GL_ARRAY_BUFFER, state->buf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data),
                             vertex_data, GL_STATIC_DRAW);

        // Upload vertex data to a buffer
        glBindBuffer(GL_ARRAY_BUFFER, state->particleBuffer);

        // Create Vertex Buffer Object (VBO)
	glBufferData(                                       // Fill bound buffer with particles
		     GL_ARRAY_BUFFER,                       // Buffer target
		     sizeof(state->particles),             // Buffer data size
		     state->particles,                     // Buffer data pointer
		     GL_DYNAMIC_DRAW);                       // Usage - Data never changes; used for drawing
        check();
}


static void draw_particle_to_texture(CUBE_STATE_T *state, GLfloat cx, GLfloat cy, GLfloat x, GLfloat y)
{
        // Draw the particle to a texture
        glBindFramebuffer(GL_FRAMEBUFFER,state->tex_fb);
        check();
        glClearColor ( 0.0, 0.0, 0.0, 1.0 );
        glClear(GL_COLOR_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, state->buf);
        check();
        glUseProgram ( state->program );
        check();
        glBindTexture(GL_TEXTURE_2D,state->tex);
        check();
        glUniform1i(state->unif_tex, 0); // I don't really understand this part, perhaps it relates to active texture?
	float alpha = 0.0f * M_PI /180.0f;
        float scaling = 0.5f;
        const GLfloat rotationMatrix[] = {
		    scaling*cosf(alpha), -scaling*sinf(alpha), 0.0f, 0.0f,
		    scaling*sinf(alpha),  scaling*cosf(alpha), 0.0f, 0.0f,
		    0.5f,                 0.5f,                1.0f, 0.0f,
                    0.0f,                 0.0f,                0.0f, 1.0f,
	};
        glUniformMatrix4fv(state->uRotationMatrix, 1, 0, rotationMatrix);
        check();
        
        glVertexAttribPointer(state->attr_vertex, 4, GL_FLOAT, 0, 16, 0);
        glEnableVertexAttribArray(state->attr_vertex);

        glDrawArrays ( GL_TRIANGLE_FAN, 0, 4 );
        check();
	glDisableVertexAttribArray(state->attr_vertex);

        glBindBuffer(GL_ARRAY_BUFFER, 0);


        glBindTexture(GL_TEXTURE_2D, state->tex_particle);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindBuffer(GL_ARRAY_BUFFER, state->particleBuffer);

#if 1
        int remaining = state->numParticles - state->whichParticle;
	// Create Vertex Buffer Object (VBO)
	glBufferSubData(                                       // Fill bound buffer with particles
		     GL_ARRAY_BUFFER,                       // Buffer target
		     0,
                     remaining * sizeof(state->particles[0]),             // Buffer data size
		     state->particles + state->whichParticle                     // Buffer data pointer
		     );                       // Usage - Data never changes; used for drawing
        check();
	// Create Vertex Buffer Object (VBO)
	glBufferSubData(                                       // Fill bound buffer with particles
		     GL_ARRAY_BUFFER,                       // Buffer target
		     remaining * sizeof(state->particles[0]),
                     state->whichParticle * sizeof(state->particles[0]),             // Buffer data size
		     state->particles                     // Buffer data pointer
		     );                       // Usage - Data never changes; used for drawing
        check();
#else
	// Create Vertex Buffer Object (VBO)
	glBufferData(                                       // Fill bound buffer with particles
		     GL_ARRAY_BUFFER,                       // Buffer target
		     sizeof(state->particles),             // Buffer data size
		     state->particles,                     // Buffer data pointer
		     GL_DYNAMIC_DRAW);                       // Usage - Data never changes; used for drawing
        check();
#endif
        glUseProgram ( state->program_particle );
        check();
        // uniforms
	const GLfloat projectionMatrix[] = {
		    1.0f, 0.0f, 0.0f, 0.0f,
		    0.0f, cx/cy, 0.0f, 0.0f,
		    0.0f, 0.0f, 1.0f, 0.0f,
		    (x-cx)/cx, (y-cy)/cy, 0.0f, 1.0f
	};
        glUniformMatrix4fv(state->uProjectionMatrix, 1, 0, projectionMatrix);
        glUniform1i(state->uTexture, 0); // first currently bound texture "GL_TEXTURE0"
        check();

	// Attributes
	glEnableVertexAttribArray(state->aPos);
	glVertexAttribPointer(state->aPos,                    // Set pointer
                      4,                                        // One component per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, pos)));      // Start from "theta" offset within bound buffer

	glEnableVertexAttribArray(state->aShade);
	glVertexAttribPointer(state->aShade,                // Set pointer
                      4,                                        // Three components per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, shade)));      // Start from "shade" offset within bound buffer

	// Draw particles
	glDrawArrays(GL_POINTS, 0, state->numParticles);

	glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D,0);

	glDisableVertexAttribArray(state->aPos);
        glDisableVertexAttribArray(state->aShade);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check();
               
        glFlush();
        glFinish();
        check();
}
        
static void draw_triangles(CUBE_STATE_T *state, GLfloat cx, GLfloat cy, GLfloat x, GLfloat y)
{
        // Now render to the main frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        // Clear the background (not really necessary I suppose)
        glClearColor ( 0.0, 1.0, 1.0, 1.0 );
        glClear(GL_COLOR_BUFFER_BIT);
        check();
        
        glBindBuffer(GL_ARRAY_BUFFER, state->buf);
        check();
        glUseProgram ( state->program );
        check();
        glBindTexture(GL_TEXTURE_2D,state->tex);
        check();
        glUniform1i(state->unif_tex, 0);  // first currently bound texture "GL_TEXTURE0"

	const GLfloat rotationMatrix[] = {
		    0.5f, 0.0f, 0.5f, 0.0f,
		    0.0f, 0.5f, 0.5f, 0.0f,
		    0.0f, 0.0f, 1.0f, 0.0f,
		    0.0f, 0.0f, 0.0f, 1.0f,
	};
        glUniformMatrix4fv(state->uRotationMatrix, 1, 0, rotationMatrix);

        check();
        
        glVertexAttribPointer(state->attr_vertex, 4, GL_FLOAT, 0, 16, 0);
        glEnableVertexAttribArray(state->attr_vertex);

        glDrawArrays ( GL_TRIANGLE_FAN, 0, 4 );
        check();
	glDisableVertexAttribArray(state->attr_vertex);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //glFlush();
        //glFinish();
        check();
        
        eglSwapBuffers(state->display, state->surface);
        check();
}

//==============================================================================

static float randrange(float min, float max)
{
   return min + rand() * ((max-min) / RAND_MAX);
}

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

int main ()
{
   int i, terminate = 0;
   GLfloat cx, cy;
   bcm_host_init();

   // Clear application state
   memset( state, 0, sizeof( *state ) );
state->verbose = 1;

  state->numWinds = 1;
  state->numEmitters = min(30, NUMEMITTERS);
  state->numParticles = min(2000, NUM_PARTICLES);
  state->size = 50.0f;
  state->windSpeed = 20.0f;
  state->emitterSpeed = 15.0f;
  state->particleSpeed = 10.0f;
  state->blur = 40.0f;
  state->eVel = state->emitterSpeed * 0.01f;

  for (i = 0; i < NUMCONSTS; ++i) {
    state->_ct[i] = randrange(0.0f, M_PI * 2.0f);
    state->_cv[i] = randrange(0.0f, 0.00005f * state->windSpeed * state->windSpeed) + 
                    0.00001f * state->windSpeed * state->windSpeed;
  }
    for (i=0; i<NUMEMITTERS; i++)
    {
      state->emitter[i].x = randrange(0.0f, 60.0f) - 30.0f;
      state->emitter[i].y = randrange(0.0f, 60.0f) - 30.0f,
      state->emitter[i].z = randrange(0.0f, 30.0f) - 15.0f;
    }

   // Start OGLES
   init_ogl(state);
   init_shaders(state);
   cx = state->screen_width/2;
   cy = state->screen_height/2;

   int frames = 0;
   uint64_t ts = GetTimeStamp();
   while (!terminate)
   {
	// update constants
	for (i = 0; i < NUMCONSTS; ++i) {
		state->_ct[i] += state->_cv[i];
		if (state->_ct[i] > M_PI * 2.0f)
		  state->_ct[i] -= M_PI * 2.0f;
		state->_c[i] = cosf(state->_ct[i]);
	}

	// calculate emissions
	for (i = 0; i < state->numEmitters; ++i) {
		// emitter moves toward viewer
		state->emitter[i].z += state->eVel;
		if (state->emitter[i].z > 15.0f) {	// reset emitter
		  state->emitter[i].x = randrange(0.0f, 60.0f) - 30.0f;
		  state->emitter[i].y = randrange(0.0f, 60.0f) - 30.0f,
		  state->emitter[i].z = -15.0f;
		}
                Particle *p = state->particles + state->whichParticle;
                p->pos[0] = state->emitter[i].x;
		p->pos[1] = state->emitter[i].y;
		p->pos[2] = state->emitter[i].z;
		p->pos[3] = 1.0;
		p->shade[3] = 1.0f;

		++state->whichParticle;
		if (state->whichParticle >= state->numParticles)
		  state->whichParticle = 0;
	}

	// calculate particle positions and colors
	// first modify constants that affect colors
	state->_c[6] *= 9.0f / state->particleSpeed;
	state->_c[7] *= 9.0f / state->particleSpeed;
	state->_c[8] *= 9.0f / state->particleSpeed;
	// then update each particle
	float pVel = state->particleSpeed * 0.01f;
	for (i = 0; i < state->numParticles; ++i) {
                Particle *p = state->particles + i;
		// store old positions
		float x = p->pos[0];
		float y = p->pos[1];
		float z = p->pos[2];
//if (p->pos[0] + p->pos[1] + p->pos[2] != 0.0f) printf("(%f %f %f), (%f %f %f), (%f %f %f) pvel:%f\n", p->shade[0], p->shade[1], p->shade[2], p->pos[0], p->pos[1], p->pos[2], state->_c[6], state->_c[7], state->_c[8], pVel);
		// make new positions
		p->pos[0] = x + (state->_c[0] * y + state->_c[1] * z) * pVel;
		p->pos[1] = y + (state->_c[2] * z + state->_c[3] * x) * pVel;
		p->pos[2] = z + (state->_c[4] * x + state->_c[5] * y) * pVel;
		// calculate colors
		p->shade[0] = abs((p->pos[0] - x) * state->_c[6]);
		p->shade[1] = abs((p->pos[1] - y) * state->_c[7]);
		p->shade[2] = abs((p->pos[2] - z) * state->_c[8]);
	}

      draw_particle_to_texture(state, cx, cy, cx, cy);
      draw_triangles(state, cx, cy, cx, cy);

    frames++;
    uint64_t ts2 = GetTimeStamp();
    if (ts2 - ts > 1e6)
    {
       printf("%d fps\n", frames);
       ts += 1e6;
       frames = 0;
    }
   }
   return 0;
}

