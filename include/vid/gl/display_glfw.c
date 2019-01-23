/*
   This file is part of corona-13.

   corona-13 is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   corona-13 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with corona-13. If not, see <http://www.gnu.org/licenses/>.
*/

#include "display.h"
#include "corona.h"
#include "heli.h"
#include "camera.h"
#include "hud.h"
#include "fbo.h"
#include "geo.h"
#include "half.h"
#include "terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>


struct GLFWwindow;
typedef struct display_t
{
	int isShuttingDown;
  int width;
  int height;
  char msg[256];
  int msg_len;
  int msg_x, msg_y;

  unsigned int program_blit_texture, program_draw_texture, program_taa, vao_empty;
  unsigned int program_grade;
  unsigned int program_draw_hud, vao_hud, vbo_hud;
  hud_t hud;
  struct GLFWwindow *window;
  fbo_t *fbo0, *fbo1; // ping pong fbos for taa
  fbo_t *fbo;         // current pointer
  fbo_t *terrain;     // ray traced terrain
  fbo_t *fsb;         // full screen buffer for post processing
  fbo_t *cube[4];     // cubemap for cockpit/geo

  uint32_t hero_program;
  uint32_t hero_texture;
  uint32_t accel_texture;   // acceleration for terrain
  geo_t *hero_fuselage;     // aircraft model fuselage
  geo_t *hero_rotor;        // aircraft model main rotor
  geo_t *landing_pad;       // landing pad

  uint32_t noise_texture[3]; // precomputed noise textures

  void (*onKeyDown)(keycode_t);
  void (*onKeyPressed)(keycode_t);
  void (*onKeyUp)(keycode_t);
  void (*onMouseButtonUp)(mouse_t);
  void (*onMouseButtonDown)(mouse_t);
  void (*onMouseMove)(mouse_t);
  void (*onActivate)(char);
  void (*onClose)();
}
display_t;


#define keyMapSize 512

static keycode_t normalKeys[keyMapSize];
// keycode_t functionKeys[keyMapSize];
// char keyIsPressed[keyMapSize];
// char keyIsReleased[keyMapSize];

int keyMapsInitialized = 0;
const unsigned char font9x16[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,63,176,63,176,30,0,0,0,0,0,0,0,0,0,112,0,120,0,0,0,0,0,120,0,112,0,0,0,0,0,4,64,31,240,31,240,4,64,
	4,64,31,240,31,240,4,64,0,0,28,96,62,48,34,16,226,28,226,28,51,240,25,224,0,0,0,0,24,48,24,96,0,192,1,128,3,0,6,0,12,48,24,48,0,0,1,224,27,240,62,16,39,16,61,224,27,240,2,16,0,0,0,0,
	0,0,8,0,120,0,112,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,192,31,224,48,48,32,16,0,0,0,0,0,0,0,0,0,0,32,16,48,48,31,224,15,192,0,0,0,0,0,0,1,0,5,64,7,192,3,128,3,128,
	7,192,5,64,1,0,0,0,0,0,1,0,1,0,7,192,7,192,1,0,1,0,0,0,0,0,0,0,0,0,0,8,0,120,0,112,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,
	0,0,0,0,0,48,0,48,0,0,0,0,0,0,0,0,0,48,0,96,0,192,1,128,3,0,6,0,12,0,0,0,0,0,15,192,31,224,48,48,35,16,48,48,31,224,15,192,0,0,0,0,0,0,8,16,24,16,63,240,63,240,0,16,
	0,16,0,0,0,0,16,112,48,240,33,144,35,16,38,16,60,48,24,48,0,0,0,0,16,32,48,48,32,16,34,16,34,16,63,240,29,224,0,0,0,0,3,0,7,0,13,0,25,16,63,240,63,240,1,16,0,0,0,0,62,32,62,48,
	34,16,34,16,34,16,35,240,33,224,0,0,0,0,15,224,31,240,50,16,34,16,34,16,35,240,1,224,0,0,0,0,48,0,48,0,33,240,35,240,38,0,60,0,56,0,0,0,0,0,29,224,63,240,34,16,34,16,34,16,63,240,29,224,
	0,0,0,0,28,0,62,16,34,16,34,16,34,48,63,224,31,192,0,0,0,0,0,0,0,0,0,0,12,96,12,96,0,0,0,0,0,0,0,0,0,0,0,0,0,16,12,112,12,96,0,0,0,0,0,0,0,0,0,0,1,0,3,128,
	6,192,12,96,24,48,16,16,0,0,0,0,0,0,4,128,4,128,4,128,4,128,4,128,4,128,0,0,0,0,0,0,16,16,24,48,12,96,6,192,3,128,1,0,0,0,0,0,24,0,56,0,32,0,33,176,35,176,62,0,28,0,0,0,
	0,0,15,224,31,240,16,16,19,208,19,208,31,208,15,128,0,0,0,0,7,240,15,240,25,0,49,0,25,0,15,240,7,240,0,0,0,0,32,16,63,240,63,240,34,16,34,16,63,240,29,224,0,0,0,0,15,192,31,224,48,48,32,16,
	32,16,48,48,24,96,0,0,0,0,32,16,63,240,63,240,32,16,48,48,31,224,15,192,0,0,0,0,32,16,63,240,63,240,34,16,39,16,48,48,56,112,0,0,0,0,32,16,63,240,63,240,34,16,39,0,48,0,56,0,0,0,0,0,
	15,192,31,224,48,48,33,16,33,16,49,224,25,240,0,0,0,0,63,240,63,240,2,0,2,0,2,0,63,240,63,240,0,0,0,0,0,0,0,0,32,16,63,240,63,240,32,16,0,0,0,0,0,0,0,224,0,240,0,16,32,16,63,240,
	63,224,32,0,0,0,0,0,32,16,63,240,63,240,3,0,7,128,60,240,56,112,0,0,0,0,32,16,63,240,63,240,32,16,0,16,0,48,0,112,0,0,0,0,63,240,63,240,28,0,14,0,28,0,63,240,63,240,0,0,0,0,63,240,
	63,240,28,0,14,0,7,0,63,240,63,240,0,0,0,0,31,224,63,240,32,16,32,16,32,16,63,240,31,224,0,0,0,0,32,16,63,240,63,240,34,16,34,0,62,0,28,0,0,0,0,0,31,224,63,240,32,16,32,48,32,28,63,252,
	31,228,0,0,0,0,32,16,63,240,63,240,34,0,35,0,63,240,28,240,0,0,0,0,24,96,60,112,38,16,34,16,35,16,57,240,24,224,0,0,0,0,0,0,56,0,48,16,63,240,63,240,48,16,56,0,0,0,0,0,63,224,63,240,
	0,16,0,16,0,16,63,240,63,224,0,0,0,0,63,128,63,192,0,96,0,48,0,96,63,192,63,128,0,0,0,0,63,224,63,240,0,112,3,192,0,112,63,240,63,224,0,0,0,0,48,48,60,240,15,192,7,128,15,192,60,240,48,48,
	0,0,0,0,0,0,60,0,62,16,3,240,3,240,62,16,60,0,0,0,0,0,56,112,48,240,33,144,35,16,38,16,60,48,56,112,0,0,0,0,0,0,0,0,63,240,63,240,32,16,32,16,0,0,0,0,0,0,28,0,14,0,7,0,
	3,128,1,192,0,224,0,112,0,0,0,0,0,0,0,0,32,16,32,16,63,240,63,240,0,0,0,0,0,0,16,0,48,0,96,0,192,0,96,0,48,0,16,0,0,0,0,0,0,4,0,4,0,4,0,4,0,4,0,4,0,4,0,4,
	0,0,0,0,0,0,96,0,112,0,16,0,0,0,0,0,0,0,0,0,0,224,5,240,5,16,5,16,7,224,3,240,0,16,0,0,0,0,32,16,63,240,63,224,4,16,6,16,3,240,1,224,0,0,0,0,3,224,7,240,4,16,4,16,
	4,16,6,48,2,32,0,0,0,0,1,224,3,240,6,16,36,16,63,224,63,240,0,16,0,0,0,0,3,224,7,240,5,16,5,16,5,16,7,48,3,32,0,0,0,0,0,0,2,16,31,240,63,240,34,16,48,0,24,0,0,0,0,0,
	3,228,7,246,4,18,4,18,3,254,7,252,4,0,0,0,0,0,32,16,63,240,63,240,2,0,4,0,7,240,3,240,0,0,0,0,0,0,0,0,4,16,55,240,55,240,0,16,0,0,0,0,0,0,0,0,0,4,0,6,0,2,4,2,
	55,254,55,252,0,0,0,0,32,16,63,240,63,240,1,128,3,192,6,112,4,48,0,0,0,0,0,0,0,0,32,16,63,240,63,240,0,16,0,0,0,0,0,0,7,240,7,240,6,0,3,240,3,240,6,0,7,240,3,240,0,0,4,0,
	7,240,3,240,4,0,4,0,7,240,3,240,0,0,0,0,3,224,7,240,4,16,4,16,4,16,7,240,3,224,0,0,0,0,4,2,7,254,3,254,4,18,4,16,7,240,3,224,0,0,0,0,3,224,7,240,4,16,4,18,3,254,7,254,
	4,2,0,0,0,0,4,16,7,240,3,240,6,16,4,0,6,0,2,0,0,0,0,0,3,32,7,176,4,144,4,144,4,144,6,240,2,96,0,0,0,0,4,0,4,0,31,224,63,240,4,16,4,48,0,32,0,0,0,0,7,224,7,240,
	0,16,0,16,7,224,7,240,0,16,0,0,0,0,7,192,7,224,0,48,0,16,0,48,7,224,7,192,0,0,0,0,7,224,7,240,0,48,0,224,0,224,0,48,7,240,7,224,0,0,4,16,6,48,3,96,1,192,1,192,3,96,6,48,
	4,16,0,0,7,226,7,242,0,18,0,18,0,22,7,252,7,248,0,0,0,0,6,48,6,112,4,208,5,144,7,16,6,48,4,48,0,0,0,0,0,0,2,0,2,0,31,224,61,240,32,16,32,16,0,0,0,0,0,0,0,0,0,0,
	62,248,62,248,0,0,0,0,0,0,0,0,0,0,32,16,32,16,61,240,31,224,2,0,2,0,0,0,0,0,32,0,96,0,64,0,96,0,32,0,96,0,64,0,0,0,0,0,1,224,3,224,6,32,12,32,6,32,3,224,1,224,0,0};

int initializeKeyMaps()
{
	for (int i = 0; i < keyMapSize; ++i) {
		normalKeys[i] = KeyUndefined;
		// functionKeys[i] = KeyUndefined;
	}

	normalKeys[GLFW_KEY_LEFT] = KeyLeft;
	normalKeys[GLFW_KEY_RIGHT] = KeyRight;
	normalKeys[GLFW_KEY_UP] = KeyUp;
	normalKeys[GLFW_KEY_DOWN] = KeyDown;
	normalKeys[GLFW_KEY_SPACE] = KeySpace;
	normalKeys[GLFW_KEY_COMMA] = KeyComma;
	normalKeys[GLFW_KEY_PERIOD] = KeyPeriod;
	normalKeys[GLFW_KEY_SLASH] = KeySlash;
	normalKeys[GLFW_KEY_0] = KeyZero;
	normalKeys[GLFW_KEY_1] = KeyOne;
	normalKeys[GLFW_KEY_2] = KeyTwo;
	normalKeys[GLFW_KEY_3] = KeyThree;
	normalKeys[GLFW_KEY_4] = KeyFour;
	normalKeys[GLFW_KEY_5] = KeyFive;
	normalKeys[GLFW_KEY_6] = KeySix;
	normalKeys[GLFW_KEY_7] = KeySeven;
	normalKeys[GLFW_KEY_8] = KeyEight;
	normalKeys[GLFW_KEY_9] = KeyNine;
	normalKeys[GLFW_KEY_SEMICOLON] = KeySemiColon;
	normalKeys[GLFW_KEY_EQUAL] = KeyEquals;
	normalKeys[GLFW_KEY_A] = KeyA;
	normalKeys[GLFW_KEY_B] = KeyB;
	normalKeys[GLFW_KEY_C] = KeyC;
	normalKeys[GLFW_KEY_D] = KeyD;
	normalKeys[GLFW_KEY_E] = KeyE;
	normalKeys[GLFW_KEY_F] = KeyF;
	normalKeys[GLFW_KEY_G] = KeyG;
	normalKeys[GLFW_KEY_H] = KeyH;
	normalKeys[GLFW_KEY_I] = KeyI;
	normalKeys[GLFW_KEY_J] = KeyJ;
	normalKeys[GLFW_KEY_K] = KeyK;
	normalKeys[GLFW_KEY_L] = KeyL;
	normalKeys[GLFW_KEY_M] = KeyM;
	normalKeys[GLFW_KEY_N] = KeyN;
	normalKeys[GLFW_KEY_O] = KeyO;
	normalKeys[GLFW_KEY_P] = KeyP;
	normalKeys[GLFW_KEY_Q] = KeyQ;
	normalKeys[GLFW_KEY_R] = KeyR;
	normalKeys[GLFW_KEY_S] = KeyS;
	normalKeys[GLFW_KEY_T] = KeyT;
	normalKeys[GLFW_KEY_U] = KeyU;
	normalKeys[GLFW_KEY_V] = KeyV;
	normalKeys[GLFW_KEY_W] = KeyW;
	normalKeys[GLFW_KEY_X] = KeyX;
	normalKeys[GLFW_KEY_Y] = KeyY;
	normalKeys[GLFW_KEY_Z] = KeyZ;
	normalKeys[GLFW_KEY_LEFT_BRACKET] = KeyOpenBracket;
	normalKeys[GLFW_KEY_BACKSLASH] = KeyBackSlash;
	normalKeys[GLFW_KEY_RIGHT_BRACKET] = KeyCloseBracket;
	return 1;
}

static void
glfw_error_callback(int error, const char *description)
{
	fprintf(stderr, "glfw error [%d]: %s\n", error, description);
}

static inline void recompile(display_t *d);
static void
glfw_key_callback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
  display_t *d = glfwGetWindowUserPointer(win);
  switch(key) {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(win, GL_TRUE);
      d->isShuttingDown = 1;
      if(d->onClose)
        d->onClose();
      break;
    default:
      if(action == GLFW_PRESS)
      {
        if(key < keyMapSize && d->onKeyDown)
          d->onKeyDown(normalKeys[key]);
        if(normalKeys[key] == KeyR)
          recompile(d);
      }
      else if(action == GLFW_RELEASE)
      {
        if(key < keyMapSize && d->onKeyUp)
          d->onKeyUp(normalKeys[key]);
      }
      break;
  }
}

static void
glfw_button_callback(GLFWwindow *win, int button, int action, int mods)
{
  display_t *d = glfwGetWindowUserPointer(win);
  double x, y;
  glfwGetCursorPos(win, &x, &y);
  mouse_t mouse;
  mouse.x = x;
  mouse.y = y;
  mouse.buttons.left   = glfwGetMouseButton(win, 0);
  mouse.buttons.middle = glfwGetMouseButton(win, 1);
  mouse.buttons.right  = glfwGetMouseButton(win, 2);
  if(action == GLFW_PRESS) {
    if (d->onMouseButtonDown) d->onMouseButtonDown(mouse);
  }
  else {
    if (d->onMouseButtonUp) d->onMouseButtonUp(mouse);
  }
}

static void
glfw_motion_callback(GLFWwindow *win, double x, double y)
{
  display_t *d = glfwGetWindowUserPointer(win);

  mouse_t mouse;
  mouse.x = x;
  mouse.y = y;
  mouse.buttons.left   = glfwGetMouseButton(win, 0);
  mouse.buttons.middle = glfwGetMouseButton(win, 1);
  mouse.buttons.right  = glfwGetMouseButton(win, 1);
  if (d->onMouseMove)
    d->onMouseMove(mouse);
}

static unsigned int
compile_src(const char *src, int type)
{
  GLuint shader_obj = glCreateShader(type);

  glShaderSource(shader_obj, 1, &src, NULL);
  glCompileShader(shader_obj);
  // const char *p = "/";
  // glCompileShaderIncludeARB(shader_obj, 1, &p, 0);

  int success;
  glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetShaderInfoLog(shader_obj, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error compiling shader: %s\n", infolog);
  }

  return shader_obj;
}

static unsigned int
compile_compute_shader(const char *src)
{
  GLuint prog = glCreateProgram();

  unsigned int comp = 0;

  glAttachShader(prog, comp = compile_src(src, GL_COMPUTE_SHADER));
  glLinkProgram(prog);

  int success;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetProgramInfoLog(prog, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error linking compute shaderprogram: %s\n", infolog);
    prog = -1;
  }

  glDeleteShader(comp);

  return prog;
}


static unsigned int
compile_shader(const char *src_vertex, const char *src_fragment)
{
  GLuint prog = glCreateProgram();

  unsigned int vert = 0, frag = 0;

  glAttachShader(prog, vert = compile_src(src_vertex, GL_VERTEX_SHADER));
  glAttachShader(prog, frag = compile_src(src_fragment, GL_FRAGMENT_SHADER));
  glLinkProgram(prog);

  int success;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if(!success) {
    char infolog[2048];
    glGetProgramInfoLog(prog, sizeof infolog, NULL, infolog);
    fprintf(stderr, "error linking shaderprogram: %s\n", infolog);
    prog = -1;
  }

  glDeleteShader(vert);
  glDeleteShader(frag);

  return prog;
}

static void
gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const char *msg, GLvoid *param)
{
  const char *ssource;
  switch(source) {
    case 0x8246: ssource = "API            "; break;
    case 0x8247: ssource = "WINDOW_SYSTEM  "; break;
    case 0x8248: ssource = "SHADER_COMPILER"; break;
    case 0x8249: ssource = "THIRD_PARTY    "; break;
    case 0x824A: ssource = "APPLICATION    "; break;
    case 0x824B: ssource = "OTHER          "; break;
    default:     ssource = "INVALID        "; break;
  }

  const char *stype;

  switch(type) {
    case 0x824C: stype = "ERROR              "; break;
    case 0x824D: stype = "DEPRECATED_BEHAVIOR"; break;
    case 0x824E: stype = "UNDEFINED_BEHAVIOR "; break;
    case 0x824F: stype = "PORTABILITY        "; break;
    case 0x8250: stype = "PERFORMANCE        "; break;
    case 0x8251: stype = "OTHER              "; break;
    default:     stype = "INVALID            "; break;
  }

  const char *sseverity;

  switch(severity) {
    case 0x9146: sseverity = "HIGH   "; break;
    case 0x9147: sseverity = "MEDIUM "; break;
    case 0x9148: sseverity = "LOW    "; break;
    default:     sseverity = "INVALID"; break;
  }

  fprintf(stderr, "%s %s %s: %s\n", sseverity, stype, ssource, msg);
}

static inline void recompile(display_t *d)
{
  char *vert_shader = read_file("res/shaders/fstri.vert", 0);
  // char *frag_shader = read_file("trace-dbg.frag", 0);
  char *frag_shader = read_file("trace2.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->program_draw_texture = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);

  vert_shader = read_file("hud.vert", 0);
  frag_shader = read_file("hud.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->program_draw_hud = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);

  vert_shader = read_file("res/shaders/fstri.vert", 0);
  frag_shader = read_file("taa.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->program_taa = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);

  vert_shader = read_file("res/shaders/fstri.vert", 0);
  frag_shader = read_file("blit.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->program_blit_texture = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);

  vert_shader = read_file("res/shaders/fstri.vert", 0);
  frag_shader = read_file("res/shaders/hud_horizon.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->program_grade = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);

  vert_shader = read_file("res/hero/hero.vert", 0);
  frag_shader = read_file("res/hero/hero.frag", 0);
  assert(vert_shader);
  assert(frag_shader);
  d->hero_program = compile_shader(
      vert_shader, frag_shader);
  free(vert_shader);
  free(frag_shader);
}

#if 0
static void
load_shader_headers(const char *path, const char *base_path)
{
  DIR *dir = opendir(path);
  if(!dir)
    return;

  char buf[1024];
  char buf_base[1024];

  struct dirent *d;
  while((d = readdir(dir))) {
    if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
      continue;
    snprintf(buf, sizeof buf, "%s/%s", path, d->d_name);
    snprintf(buf_base, sizeof buf_base, "%s/%s", base_path, d->d_name);

    if(d->d_type == DT_DIR) {
      load_shader_headers(buf, base_path);
    }
    else if(d->d_type == DT_REG) {
      FILE *f = fopen(buf, "r");
      fseek(f, 0, SEEK_END);
      size_t size = ftell(f);
      fseek(f, 0, SEEK_SET);
      char *hdr = malloc(size+1);
      fread(hdr, size, 1, f);
      hdr[size] = 0;
      fclose(f);
      glNamedStringARB(GL_SHADER_INCLUDE_ARB, -1, buf_base, -1, hdr); 
      free(hdr);
    }
  }
}
#endif

display_t *display_open(const char title[], int width, int height)
{
  if(!keyMapsInitialized) keyMapsInitialized = initializeKeyMaps();
  display_t *d = (display_t*) malloc(sizeof(display_t));
  memset(d, sizeof(*d), 0);

  d->width = width;
  d->height = height;
  d->isShuttingDown = 0;
  d->onKeyDown = NULL;
  d->onKeyPressed = NULL;
  d->onKeyUp = NULL;
  d->onMouseButtonUp = NULL;
  d->onMouseButtonDown = NULL;
  d->onMouseMove = NULL;
  d->onActivate = NULL;
  d->onClose = NULL;
  d->msg[0] = '\0';
  d->msg_len = 0;

  glfwSetErrorCallback(glfw_error_callback);
  if(!glfwInit()) {
    fprintf(stderr, "error initializing glfw\n");
    free(d);
    return NULL;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // anti aliased lines for hud
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // really want 5 for bindtextureunit
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // seems to be the newest the quadro 5800 supports :(
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_RED_BITS, 10);
  // glfwWindowHint(GLFW_GREEN_BITS, 10);
  // glfwWindowHint(GLFW_BLUE_BITS, 10);
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  // glfwWindowHint(GLFW_ALPHA_BITS, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  if(!(d->window = glfwCreateWindow(width, height, title, NULL, NULL)))
  {
    fprintf(stderr, "error creating window\n");
    glfwTerminate();
    free(d);
    return NULL;
  }

  glfwMakeContextCurrent(d->window);
  glfwSetKeyCallback(d->window, glfw_key_callback);
  glfwSetCursorPosCallback(d->window, glfw_motion_callback);
  glfwSetMouseButtonCallback(d->window, glfw_button_callback);

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if(err != GLEW_OK) {
    fprintf(stderr,  "Error: %s\n", glewGetErrorString(err));
    return NULL;
  }
  while(glGetError() != GL_NO_ERROR);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glEnable(GL_DEBUG_OUTPUT);

  glDebugMessageCallback((GLDEBUGPROC) gl_error_callback, NULL);
  //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
  GLuint mask_ids[] = {
    131076, // Generic Vertex Attrib foo
    131185, // memory type for buffer operations
  };
  glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE,
      sizeof(mask_ids) / sizeof(mask_ids[0]), mask_ids, GL_FALSE);


  fbo_init_t fi = {{0}};
  fi.color[0] = GL_RGBA16F; // colour
  fi.color[1] = GL_RG16F;   // motion vectors
  fi.width = width;
  fi.height = height;
  // fi.depth = GL_DEPTH_COMPONENT16;
  fi.depth = GL_DEPTH_COMPONENT32F;
  // full screen buffers for taa and post:
  d->fsb = fbo_init(&fi);
  d->fbo0 = fbo_init(&fi);
  d->fbo1 = fbo_init(&fi);

  // terrain buffer is mip mapped, so we can use them for diffuse lighting
  fi.color_mip = 1;
  // terrain can be reduced res, will be upscaled
  fi.width  = width/rt.rt_lod;
  fi.height = height/rt.rt_lod;
  // environment will be ray traced into this:
  d->terrain = fbo_init(&fi);
  d->fbo = d->fbo0;

  // init cube map for fisheye geo
  fi.width = 1.3*height;  // expand a bit for better resolution
  fi.height = 1.3*height; // because we'll resample it
  fi.color_mip = 0;
  fi.depth = GL_DEPTH_COMPONENT32F;
  fi.color_mip = 0;
  for(int k=0;k<2;k++)
    d->cube[k] = fbo_init(&fi);

  // we won't use the depth textures for shadow mapping:
  glBindTexture(GL_TEXTURE_2D, d->terrain->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  glBindTexture(GL_TEXTURE_2D, d->cube[0]->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  glBindTexture(GL_TEXTURE_2D, d->cube[1]->tex_depth);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

  glGenTextures(1, &d->hero_texture);
  glBindTexture(GL_TEXTURE_2D, d->hero_texture);
  {
    FILE *f = fopen("res/hero/diffuse-small.ppm", "rb");
    if(f)
    {
      int width, height;
      fscanf(f, "P6\n%d %d\n255", &width, &height);
      fprintf(stderr, "[hero] loading hero texture %dx%d\n", width, height);
      fgetc(f);
      uint8_t *pixels = malloc(sizeof(uint8_t)*3*width*height);
      fread(pixels, 1, sizeof(uint8_t)*3*width*height, f);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, width, height);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
      fclose(f);
      free(pixels);
    }
  }


  glGenTextures(1, &d->accel_texture);
  glBindTexture(GL_TEXTURE_2D, d->accel_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  {
    int w = 4096, h = 4096;
    glTexStorage2D(GL_TEXTURE_2D, 12, GL_R16F, w, h);
    uint16_t *pixels = malloc(sizeof(uint16_t)*w*h);

    for(int j=0;j<h;j++) for(int i=0;i<w;i++)
    {
      float x = (i/(w-1.0)-.5)*1000.0;
      float y = (j/(h-1.0)-.5)*1000.0;
      float h = terrain_height(x, y);
      pixels[j*w+i] = float_to_half(h);
    }
    for(int l=0;l<12;l++)
    {
      glTexSubImage2D(GL_TEXTURE_2D, l, 0, 0, w, h, GL_RED, GL_HALF_FLOAT, pixels);
      h/=2;
      w/=2;
      for(int j=0;j<h;j++) for(int i=0;i<w;i++)
      {
        pixels[j*w+i] = MAX(
            MAX(pixels[(2*j+0)*(2*w)+2*i+0],
                pixels[(2*j+0)*(2*w)+2*i+1]),
            MAX(pixels[(2*j+1)*(2*w)+2*i+0],
                pixels[(2*j+1)*(2*w)+2*i+1]));
      }
    }
    free(pixels);
  }

  recompile(d);
  glGenVertexArrays(1, &d->vao_empty);

  glGenVertexArrays(1, &d->vao_hud);
  glBindVertexArray(d->vao_hud);
  glGenBuffers(1, &d->vbo_hud);

  glBindBuffer(GL_ARRAY_BUFFER, d->vbo_hud);

  glNamedBufferStorage(d->vbo_hud, sizeof(d->hud.lines), d->hud.lines, GL_DYNAMIC_STORAGE_BIT);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glEnableVertexArrayAttrib(d->vao_hud, 0);
  glBindAttribLocation(d->program_draw_hud, 0, "position");

  // load_shader_headers("res/shader", "");

  d->hero_fuselage = geo_init("res/gazelle", d->hero_program);
  d->hero_rotor = geo_init("res/rotor", d->hero_program);
  // d->hero = geo_init("res/artifact", d->hero_program);
  // d->hero = geo_init("res/Icosphere", d->hero_program);
  d->landing_pad = geo_init("res/landing-pad", d->hero_program);

  { // precompute 3d noises:
    fprintf(stderr, "[main] loading precomputed noise textures..\n");
    glGenTextures(3, d->noise_texture);
    glBindTexture(GL_TEXTURE_3D, d->noise_texture[0]);
    uint32_t w = 128;
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, w, w, w);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    size_t len;
    uint8_t *pixels = read_file("tools/noise/worley.tex", &len);
    assert(len == 4*w*w*w);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, w, w, w, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    glBindTexture(GL_TEXTURE_3D, d->noise_texture[1]);
    w = 32;
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, w, w, w);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    pixels = read_file("tools/noise/erosion.tex", &len);
    assert(len == 4*w*w*w);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, w, w, w, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    // 3rd texture: 2D 128x128 curl noise dimensions:
    glBindTexture(GL_TEXTURE_2D, d->noise_texture[2]);
    w = 128;
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, w);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    pixels = read_file("tools/noise/curl.tex", &len);
    assert(len == 4*w*w);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#if 0
    fprintf(stderr, "[main] precomputing noise textures..\n");
    glGenTextures(3, d->noise_texture);
    char *shader = read_file("res/shaders/noise.glsl", 0);
    assert(shader);
    GLuint program = compile_compute_shader(shader);
    free(shader);
    glUseProgram(program);
    // first texture: 3D rgba 128x128x128 perlin-worley + 3 scales worley
    glUniform1i(glGetUniformLocation(program, "u_mode"), 0);
    glBindTexture(GL_TEXTURE_3D, d->noise_texture[0]);
    uint32_t w = 128, h = 128, b = 128;
    // uint32_t w = 8, h = 8, b = 8;
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, w, h, b);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glBindTextureUnit(0, d->noise_texture[0]);
    glBindImageTexture(0, d->noise_texture[0], 0, GL_FALSE, 0 , GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(w, h, b);

#if 0
    // 2nd texture: 3D rgb 32x32x32 3 worley scales
    glUniform1i(glGetUniformLocation(program, "u_mode"), 1);
    glBindTexture(GL_TEXTURE_3D, d->noise_texture[1]);
    w = 32; h = 32; b = 32;
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB16F, w, h, b);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTextureUnit(0, d->noise_texture[1]);
    glDispatchCompute(w, h, b);

    // 3rd texture: 2D 128x128 curl noise dimensions:
    glUniform1i(glGetUniformLocation(program, "u_mode"), 2);
    glBindTexture(GL_TEXTURE_3D, d->noise_texture[2]);
    w = 128; h = 128; b = 1;
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, w, h);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTextureUnit(1, d->noise_texture[2]);
    glDispatchCompute(w, h, b);
#endif
    fprintf(stderr, "[main] done.\n");
#endif
  }
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  return d;
}

void display_close(display_t *d)
{
  geo_cleanup(d->hero_fuselage);
  geo_cleanup(d->hero_rotor);
  fbo_cleanup(d->fbo0);
  fbo_cleanup(d->fbo1);
  fbo_cleanup(d->terrain);
  glfwDestroyWindow(d->window);
  glfwTerminate();

  free(d);
}

void display_pump_events(display_t *d)
{
  glfwSetWindowUserPointer(d->window, d);
  glfwPollEvents();

  int present = glfwJoystickPresent(GLFW_JOYSTICK_1);
  if(present)
  {
    int count;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
    if(count > 0)
      heli_control_cyclic_x(rt.heli, axes[0]);
    if(count > 1)
      heli_control_cyclic_z(rt.heli, -axes[1]);
    if(count > 2)
      heli_control_collective(rt.heli, .5f*(1.0 + axes[2]));
    if(count > 3)
      heli_control_tail(rt.heli, -axes[3]);
    // buttons on throttle for thrustmaster hotas:
    if(count > 4)
      heli_control_tail(rt.heli, -axes[4]);
    if(count <= 3) // no axis available for tail
    {
      const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &count);
      if(buttons[3] == GLFW_RELEASE)
        heli_control_tail(rt.heli, 0.0f);
      if(buttons[4] == GLFW_RELEASE)
        heli_control_tail(rt.heli, 0.0f);
      if(buttons[3] == GLFW_PRESS)
        heli_control_tail(rt.heli, 1.0f);
      if(buttons[4] == GLFW_PRESS)
        heli_control_tail(rt.heli, -1.0f);
    }
  }

  if(glfwWindowShouldClose(d->window)) {
    d->isShuttingDown = 1;
    if(d->onClose)
      d->onClose();
  }
}

static inline void display_hud(display_t *d)
{
  hud_init(&d->hud, rt.heli);
  glLineWidth(1.5f*d->width/1024.0f);
  glUseProgram(d->program_draw_hud);
  glNamedBufferSubData(d->vbo_hud, 0, sizeof(d->hud.lines), d->hud.lines);
  glBindVertexArray(d->vao_hud);

  glDrawArrays(GL_LINES, 0, d->hud.num_lines);
}

int display_update(display_t *d, const float *pixels, const float scale)
{
  if (d->isShuttingDown)
    return 0;

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if(d->program_draw_texture == -1) return 1;

  fbo_t *old_fbo = d->fbo == d->fbo0 ? d->fbo1 : d->fbo0;

  // half angle of horizontal field of view. pi/2 means 180 degrees total
  // and is the max we can get with only two cubemap sides
  const float hfov = 1.52;

  // rasterise geo before ray tracing
  // TODO: factor out code such that this here looks more sane (encapsulate into object + shaders)
  if(d->hero_program != -1)
  {
    // need to:
    // 1) transform model to world space orientation (model q)
    // 2) add world space position of model's center of mass (model x)
    // 3) subtract camera position (-view x)
    // 4) rotate world to camera   (inv view q)

    //   ivq * (-vx + mx + mq * v)
    // = ivq * (mx - vx) + (ivq mq) * v
    //   `------o------'   `---o--'    <= store those two (vec + quat)

    const quat_t *mq = &rt.heli->body.q;
    float mvx[3] = {
      rt.heli->body.c[0]-rt.cam->x[0],
      rt.heli->body.c[1]-rt.cam->x[1],
      rt.heli->body.c[2]-rt.cam->x[2]};
    quat_t ivq = rt.cam->q;
    quat_conj(&ivq);
    quat_transform(&ivq, mvx);
    quat_t mvq;
    quat_mul(&ivq, mq, &mvq);

    const quat_t *omq = &rt.heli->prev_q;
    float omvx[3] = {
      rt.heli->prev_x[0]-rt.cam->prev_x[0],
      rt.heli->prev_x[1]-rt.cam->prev_x[1],
      rt.heli->prev_x[2]-rt.cam->prev_x[2]};
    quat_t iovq = rt.cam->prev_q;
    quat_conj(&iovq);
    quat_transform(&iovq, omvx);
    quat_t omvq;
    quat_mul(&iovq, mq, &omvq);

    float sunv[] = {-0.624695,0.468521,-0.624695};
    normalise(sunv);
    quat_transform_inv(&rt.cam->q, sunv); // sun in camera space

    // turns out we only need L and R cube sides
    for(int k=0;k<2;k++)
    {
      fbo_bind(d->cube[k]);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      // TODO: encapsulate shaders on the other side
      glUseProgram(d->hero_program);

      glUniform1i(glGetUniformLocation(d->hero_program, "u_cube_side"), k);
      glUniform3f(glGetUniformLocation(d->hero_program, "u_sun"),
          sunv[0], sunv[1], sunv[2]);
      glUniform1f(glGetUniformLocation(d->hero_program, "u_time"), rt.frames/100.0);
      glUniform1f(glGetUniformLocation(d->hero_program, "u_hfov"), hfov);
      glUniform3f(glGetUniformLocation(d->hero_program, "u_old_pos"),
          omvx[0], omvx[1], omvx[2]);
      glUniform4f(glGetUniformLocation(d->hero_program, "u_old_q"),
          omvq.x[0], omvq.x[1], omvq.x[2], omvq.w);
      glUniform3f( glGetUniformLocation(d->hero_program, "u_pos"),
          mvx[0], mvx[1], mvx[2]);
      glUniform3f(glGetUniformLocation(d->hero_program, "u_pos_ws"),
          rt.cam->x[0], rt.cam->x[1], rt.cam->x[2]);
      glUniform4f(glGetUniformLocation(d->hero_program, "u_q"),
          mvq.x[0], mvq.x[1], mvq.x[2], mvq.w);
      glUniform1f(glGetUniformLocation(d->hero_program, "u_lod"), rt.rt_lod);
      glUniform2f(glGetUniformLocation(d->hero_program, "u_res"),
          d->fbo->width, d->fbo->height);
      glUniform1i(glGetUniformLocation(d->hero_program, "u_frame"), rt.frames);
      glBindTextureUnit(0, d->terrain->tex_color[0]);
      glBindTextureUnit(1, d->hero_texture);

      GLint rot = glGetUniformLocation(d->hero_program, "u_rotate");
      glUniform1i(rot, 0);
      geo_render(d->hero_fuselage);
      glUniform1i(rot, 1);
      geo_render(d->hero_rotor);

      // static geo:
      glUniform1i(rot, 2);
      quat_t padrot, pad_mvq, prev_pad_mvq;
      quat_init_angle(&padrot, -M_PI/2.0, 1, 0, 0);
      quat_mul(&ivq, &padrot, &pad_mvq);
      quat_mul(&iovq, &padrot, &prev_pad_mvq);
      float prev_padpos[3], padpos[3] = {-235, -167, 200};
      for(int k=0;k<3;k++) prev_padpos[k] = padpos[k] - rt.cam->prev_x[k];
      for(int k=0;k<3;k++) padpos[k] -= rt.cam->x[k];
      quat_transform(&ivq, padpos);
      quat_transform(&iovq, prev_padpos);
      glUniform3f(glGetUniformLocation(d->hero_program, "u_old_pos"),
          prev_padpos[0], prev_padpos[1], prev_padpos[2]);
      glUniform4f(glGetUniformLocation(d->hero_program, "u_old_q"),
          prev_pad_mvq.x[0], prev_pad_mvq.x[1], prev_pad_mvq.x[2], prev_pad_mvq.w);
      glUniform3f(glGetUniformLocation(d->hero_program, "u_pos"),
          padpos[0], padpos[1], padpos[2]);
      glUniform4f(glGetUniformLocation(d->hero_program, "u_q"),
          pad_mvq.x[0], pad_mvq.x[1], pad_mvq.x[2], pad_mvq.w);
      geo_render(d->landing_pad);

      glDisable(GL_DEPTH_TEST);
      fbo_unbind(d->cube[k]);
    }
  }

  // now unstretch geo to fisheye buffer
  // fbo_bind(d->fsb);
  fbo_bind(d->terrain);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if(d->program_blit_texture != -1)
  { // unstretch geo and depth combine with ray traced buf
    glUseProgram(d->program_blit_texture);
    glUniform1f(glGetUniformLocation(d->program_blit_texture, "u_lod"), rt.rt_lod);
    glUniform1f(glGetUniformLocation(d->program_blit_texture, "u_hfov"), hfov);
    glUniform1f(glGetUniformLocation(d->program_blit_texture, "u_time"), rt.time);
    glUniform3f(glGetUniformLocation(d->program_blit_texture, "u_pos"),
        rt.cam->x[0], rt.cam->x[1], rt.cam->x[2]);
    // object to world
    glUniform4f(glGetUniformLocation(d->program_blit_texture, "u_orient"),
        rt.cam->q.x[0], rt.cam->q.x[1],
        rt.cam->q.x[2], rt.cam->q.w);
    // glBindTextureUnit(0, d->terrain->tex_color[0]);
    // glBindTextureUnit(1, d->terrain->tex_color[1]);
    glBindTextureUnit(2, d->cube[0]->tex_color[0]);
    glBindTextureUnit(3, d->cube[0]->tex_color[1]);
    glBindTextureUnit(4, d->cube[1]->tex_color[0]);
    glBindTextureUnit(5, d->cube[1]->tex_color[1]);
    // glBindTextureUnit(6, d->terrain->tex_depth);
    glBindTextureUnit(7, d->cube[0]->tex_depth);
    glBindTextureUnit(8, d->cube[1]->tex_depth);

    glBindVertexArray(d->vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }
  fbo_unbind(d->terrain);

  if(d->program_draw_texture != -1)
  { // ray trace landscape:
    fbo_bind(d->fsb);
    // glClearColor(0, 0, 0, 1);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);
    glUseProgram(d->program_draw_texture);
    glUniform1f(glGetUniformLocation(d->program_draw_texture, "u_hfov"), hfov);
    glUniform2f(glGetUniformLocation(d->program_draw_texture, "u_res"),
        d->fbo->width, d->fbo->height);
    glUniform1f(glGetUniformLocation(d->program_draw_texture, "u_lod"),
        rt.rt_lod);
    glUniform1f(glGetUniformLocation(d->program_draw_texture, "u_time"),
        rt.time);
    glUniform3f(glGetUniformLocation(d->program_draw_texture, "u_pos"),
        rt.cam->x[0], rt.cam->x[1], rt.cam->x[2]);
    glUniform3f(glGetUniformLocation(d->program_draw_texture, "u_heli_pos"),
        rt.heli->body.c[0], rt.heli->body.c[1], rt.heli->body.c[2]);
    glUniform4f(glGetUniformLocation(d->program_draw_texture, "u_orient"),
        rt.cam->q.x[0], rt.cam->q.x[1], rt.cam->q.x[2], rt.cam->q.w);
    glUniform3f(glGetUniformLocation(d->program_draw_texture, "u_old_pos"),
        rt.cam->prev_x[0], rt.cam->prev_x[1], rt.cam->prev_x[2]);
    glUniform4f(glGetUniformLocation(d->program_draw_texture, "u_old_orient"),
        rt.cam->prev_q.x[0], rt.cam->prev_q.x[1],
        rt.cam->prev_q.x[2], rt.cam->prev_q.w);

    glBindTextureUnit(0, old_fbo->tex_color[0]);
    glBindTextureUnit(1, d->accel_texture);
    glBindTextureUnit(2, d->noise_texture[0]);
    glBindTextureUnit(3, d->noise_texture[1]);
    glBindTextureUnit(4, d->noise_texture[2]);
    glBindTextureUnit(5, d->terrain->tex_color[0]);
    glBindVertexArray(d->vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    // glDisable(GL_DEPTH_TEST);
    fbo_unbind(d->fsb);
  }

  // FIXME: diffuse illumination now completely broken
  // need to bind some other buffer, with only environment?
  glGenerateTextureMipmap(d->terrain->tex_color[0]);


  // taa:
  // read d->fsb and old_fbo and write to fbo
  fbo_bind(d->fbo);
  if(d->program_taa != -1)
  { // do taa from two fbo to one output fbo
    glUseProgram(d->program_taa);
    GLint res = glGetUniformLocation(d->program_taa, "u_res");
    glUniform2f(res, d->fbo->width, d->fbo->height);
    GLint lod = glGetUniformLocation(d->program_taa, "u_lod");
    glUniform1f(lod, rt.rt_lod);
    glBindTextureUnit(0, old_fbo->tex_color[0]);
    glBindTextureUnit(1, d->fsb->tex_color[0]); // current colour
    glBindTextureUnit(2, d->fsb->tex_color[1]); // current motion
    glBindVertexArray(d->vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }
  fbo_unbind(d->fbo);

  // one more pass drawing just the hud frag shader and blitting to render buffer
  if(d->program_grade != -1)
  {
    glUseProgram(d->program_grade);
    glUniform1f(glGetUniformLocation(d->program_grade, "u_hfov"), hfov);
    GLint res = glGetUniformLocation(d->program_grade, "u_res");
    glUniform2f(res, d->fbo->width, d->fbo->height);
    GLint pos = glGetUniformLocation(d->program_grade, "u_pos");
    glUniform3f(pos,
        rt.cam->x[0], rt.cam->x[1], rt.cam->x[2]);
    GLint orient = glGetUniformLocation(d->program_grade, "u_orient");
    glUniform4f(orient, // object to world
        rt.cam->q.x[0], rt.cam->q.x[1],
        rt.cam->q.x[2], rt.cam->q.w);
    glBindTextureUnit(0, d->fbo->tex_color[0]);
    // glBindTextureUnit(0, d->fsb->tex_color[0]); // XXX DEBUG no taa
    // glBindTextureUnit(0, d->terrain->tex_color[0]); // XXX DEBUG directly see terrain
    glBindVertexArray(d->vao_empty);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
  }

  display_hud(d);

  d->fbo = old_fbo;
  glfwSwapBuffers(d->window);
  return 1;
}

void display_print(display_t *d, const int px, const int py, const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  vsnprintf(d->msg, 255, msg, ap);
  va_end(ap);
  d->msg_len = strlen(d->msg);
  d->msg_x = px;
  d->msg_y = py + 15;
}

void display_register_callbacks(struct display_t *d, 
  void (*onKeyDown)(keycode_t),
  void (*onKeyPressed)(keycode_t),
  void (*onKeyUp)(keycode_t),
  void (*onMouseButtonDown)(mouse_t),
  void (*onMouseButtonUp)(mouse_t),
  void (*onMouseMove)(mouse_t),
  void (*onActivate)(char),
  void (*onClose)())
{
  d->onKeyDown = onKeyDown;
  d->onKeyPressed = onKeyPressed;
  d->onKeyUp = onKeyUp;
  d->onMouseButtonDown = onMouseButtonDown;
  d->onMouseButtonUp = onMouseButtonUp;
  d->onMouseMove = onMouseMove;
  d->onActivate = onActivate;
  d->onClose = onClose;
}
