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

#define NUM_PARTICLES 360
#define PARTICLE_WIDTH  16
#define PARTICLE_HEIGHT 16
#define min(a,b) ((a)<(b)?(a):(b)) 
#define max(a,b) ((a)<(b)?(b):(a)) 

typedef struct Particle
{
    GLfloat pos[2];
    GLfloat vel[2];
    GLfloat acc[2];
    GLfloat shade[4];
} Particle;
 
typedef struct Emitter
{
    Particle particles[NUM_PARTICLES];
    GLfloat  k;
    GLfloat  color[4];
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
   GLuint aPos, aVel, aAcc, uProjectionMatrix, uK, aShade, uColor, uTime, uTexture;

   Emitter emitter;
    float   _timeCurrent;
    float   _timeMax;
    int     _timeDirection;
  void *tex_particle_data;
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
	"attribute vec2 aPos;"
	"attribute vec2 aVel;"
	"attribute vec2 aAcc;"
        "attribute vec4 aShade;"
	""
	//"// Uniforms"
	"uniform mat4 uProjectionMatrix;"
	"uniform float uK;"
	"uniform float uTime;"
	"varying vec4 vShade;"
        ""
	"void main(void)"
	"{"
	"    vec2 xy = aPos + aVel * uTime + aAcc * uTime * uTime;"
	"    gl_Position = uProjectionMatrix * vec4(xy.x, xy.y, 0.0, 1.0);"
	"    gl_PointSize = 16.0;"
	"    vShade = vec4(aShade.xyz, aShade.a - 1e-2*uTime*uTime);"
	"}";
 
   //particle
   const GLchar *particle_fshader_source =
	//" Input from Vertex Shader"
	"varying highp vec4 vShade;"
	"" 
	//" Uniforms"
	"uniform sampler2D uTexture;"
	"uniform highp vec4 uColor;"
	""
	"void main(void)"
	"{"
	"    highp vec4 texture = texture2D(uTexture, gl_PointCoord);"
	"    highp vec4 color = uColor + vShade;"
	"    color = clamp(color, vec4(0.0), vec4(1.0));"
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
        state->aVel              = glGetAttribLocation(state->program_particle, "aVel");
        state->aAcc              = glGetAttribLocation(state->program_particle, "aAcc");
        state->uProjectionMatrix = glGetUniformLocation(state->program_particle, "uProjectionMatrix");
        state->uK                = glGetUniformLocation(state->program_particle, "uK");
	state->aShade            = glGetAttribLocation(state->program_particle, "aShade");
        state->uColor            = glGetUniformLocation(state->program_particle, "uColor");
	state->uTime             = glGetUniformLocation(state->program_particle, "uTime");
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
		     sizeof(state->emitter.particles),             // Buffer data size
		     state->emitter.particles,                     // Buffer data pointer
		     GL_STATIC_DRAW);                       // Usage - Data never changes; used for drawing
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
        glUniform1f(state->uK, state->emitter.k);
        glUniform4f(state->uColor, state->emitter.color[0], state->emitter.color[1], state->emitter.color[2], state->emitter.color[3]);

        glUniform1f(state->uTime, state->_timeCurrent);///state->_timeMax);
        glUniform1i(state->uTexture, 0); // first currently bound texture "GL_TEXTURE0"
        check();

	// Attributes
	glEnableVertexAttribArray(state->aPos);
	glVertexAttribPointer(state->aPos,                    // Set pointer
                      2,                                        // One component per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, pos)));      // Start from "theta" offset within bound buffer

	glEnableVertexAttribArray(state->aVel);
	glVertexAttribPointer(state->aVel,                    // Set pointer
                      2,                                        // One component per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, vel)));      // Start from "theta" offset within bound buffer

	glEnableVertexAttribArray(state->aAcc);
	glVertexAttribPointer(state->aAcc,                    // Set pointer
                      2,                                        // One component per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, acc)));      // Start from "theta" offset within bound buffer

	glEnableVertexAttribArray(state->aShade);
	glVertexAttribPointer(state->aShade,                // Set pointer
                      4,                                        // Three components per particle
                      GL_FLOAT,                                 // Data is floating point type
                      GL_FALSE,                                 // No fixed point scaling
                      sizeof(Particle),                         // No gaps in data
                      (void*)(offsetof(Particle, shade)));      // Start from "shade" offset within bound buffer

	// Draw particles
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

	glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D,0);

	glDisableVertexAttribArray(state->aPos);
	glDisableVertexAttribArray(state->aVel);
	glDisableVertexAttribArray(state->aAcc);
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

        glFlush();
        glFinish();
        check();
        
        eglSwapBuffers(state->display, state->surface);
        check();
}

static int get_mouse(CUBE_STATE_T *state, int *outx, int *outy)
{
    static int fd = -1;
    const int width=state->screen_width, height=state->screen_height;
    static int x=800, y=400;
    const int XSIGN = 1<<4, YSIGN = 1<<5;
    if (fd<0) {
       fd = open("/dev/input/mouse0",O_RDONLY|O_NONBLOCK);
    }
    if (fd>=0) {
        struct {char buttons, dx, dy; } m;
        while (1) {
           int bytes = read(fd, &m, sizeof m);
           if (bytes < (int)sizeof m) goto _exit;
           if (m.buttons&8) {
              break; // This bit should always be set
           }
           read(fd, &m, 1); // Try to sync up again
        }
        if (m.buttons&3)
           return m.buttons&3;
        x+=m.dx;
        y+=m.dy;
        if (m.buttons&XSIGN)
           x-=256;
        if (m.buttons&YSIGN)
           y-=256;
        if (x<0) x=0;
        if (y<0) y=0;
        if (x>width) x=width;
        if (y>height) y=height;
   }
_exit:
   if (outx) *outx = x;
   if (outy) *outy = y;
   return 0;
}       
 
//==============================================================================

static float randrange(float min, float max)
{
   return min + rand() * ((max-min) / RAND_MAX);
}

int main ()
{
   int i, terminate = 0;
   GLfloat cx, cy;
   bcm_host_init();

   // Clear application state
   memset( state, 0, sizeof( *state ) );
state->verbose = 1;
    float x = randrange( 0.0f, 1.0f) - 0.5f;
    float y = randrange( 0.0f, 1.0f);
    for(i=0; i<NUM_PARTICLES; i++)
    {
        // Assign each particle its theta value (in radians)
        state->emitter.particles[i].pos[0] = x;//(float)i / NUM_PARTICLES - 0.5f;
        state->emitter.particles[i].pos[1] = y;//randrange( 0.0f, 1.0f) - 0.5f;

        float theta = randrange(0.0f, 2.0f*M_PI);
        float vel   = randrange(0.0f, 0.25f);
        state->emitter.particles[i].vel[0] = vel*sinf(theta);
        state->emitter.particles[i].vel[1] = vel*cosf(theta);

        state->emitter.particles[i].acc[0] = 0.0f;
        state->emitter.particles[i].acc[1] = -0.1f;

        state->emitter.particles[i].shade[0] = randrange(-0.25f, 0.25f);
        state->emitter.particles[i].shade[1] = randrange(-0.25f, 0.25f);
        state->emitter.particles[i].shade[2] = randrange(-0.25f, 0.25f);
        state->emitter.particles[i].shade[3] = randrange( 0.00f, 1.00f);
    }
    state->emitter.k = 4;
    state->emitter.color[0] = 0.76f;   // Color: R
    state->emitter.color[1] = 0.12f;   // Color: G
    state->emitter.color[2] = 0.34f;   // Color: B
    state->emitter.color[3] = 0.00f;   // Color: A

	// Initialize variables
	state->_timeCurrent = 0.0f;
	state->_timeMax = 5.0f;
	state->_timeDirection = 1;

   // Start OGLES
   init_ogl(state);
   init_shaders(state);
   cx = state->screen_width/2;
   cy = state->screen_height/2;

   while (!terminate)
   {
      int x, y, b;
      b = get_mouse(state, &x, &y);
      if (b) break;
      draw_particle_to_texture(state, cx, cy, x, y);
      draw_triangles(state, cx, cy, x, y);

    if(state->_timeCurrent > state->_timeMax)
        state->_timeDirection = -1;
    else if(state->_timeCurrent < 0.0f)
        state->_timeDirection = 1;

    state->_timeCurrent += state->_timeDirection * (1.0f/30.0f);
   }
   return 0;
}

