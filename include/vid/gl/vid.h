#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "vid/gl/fbo.h"
#include "hud.h"

typedef struct sx_material_t
{
  char texname[15];   // texture file name, .3do supports 12 chars
  uint32_t texid[32]; // texture id, possibly up to 32 frames in an animation
  uint32_t tex_cnt;   // number of texture frames
  int32_t  texu;      // texture unit
  uint32_t dunno[4];  // some extra info
}
sx_material_t;

typedef struct sx_geo_t
{
  GLuint vbo[4];    // vertices, normals, uvs, indices
  GLuint vaid;
  uint32_t num_vtx;
  uint32_t num_nor;
  uint32_t num_uvs;
  uint32_t num_idx;
  uint32_t num_mat;
  GLuint program;
  GLuint mat_tex;
  sx_material_t mat[128];
  char filename[32];
}
sx_geo_t;

typedef struct sx_vid_t
{
  SDL_Window    *window;
  SDL_GLContext  context;
  SDL_Joystick  *joystick;

  uint32_t program_blit_texture, program_draw_texture,
           program_taa, vao_empty, program_grade, program_hero,
           program_draw_hud, program_hud_text,
           program_debug_line;

  uint32_t vao_hud, vbo_hud;
  sx_hud_t hud;

  uint32_t vao_hud_text, vbo_hud_text[3];
  uint32_t hud_text_chars; // number of characters, 100 max
  uint32_t hud_text_font;  // texture id
  float hud_text_vx[800], hud_text_uv[800];
  uint32_t hud_text_id[600];

  uint32_t debug_line_cnt;
  float debug_line_vx[2000];
  uint32_t vao_debug_line, vbo_debug_line;

  fbo_t *fbo0, *fbo1; // ping pong fbos for taa
  fbo_t *fbo;         // current pointer
  fbo_t *raster;      // rasterised geo, unwarped
  fbo_t *fsb;         // full screen buffer for terrain and comp
  fbo_t *cube[4];     // cubemap for rasterised geo

  uint32_t num_geo;
  sx_geo_t geo[2048];

  // terrain textures and "character" detail textures:
  uint32_t tex_terrain[8];   // col, dis, mat, ccol, cdis, cmat, acc, cacc

  // TODO: 
  uint32_t tex_skypal, tex_clouds;
}
sx_vid_t;

void sx_vid_add_debug_line(const float *v0, const float *v1);
void sx_vid_clear_debug_line();
