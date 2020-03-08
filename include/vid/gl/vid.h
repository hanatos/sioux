#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "vid/gl/fbo.h"
#include "hud.h"

typedef struct sx_material_t
{
  uint64_t tex_handle;
  uint32_t dunno[4];   // some extra info, dunno[4] has frame count for textures
}
sx_material_t;

typedef struct sx_geo_t
{
  GLuint vbo[4];      // vertices, normals, uvs, indices
  GLuint vaid;
  uint32_t num_vtx;
  uint32_t num_nor;
  uint32_t num_uvs;
  uint32_t num_idx;
  uint32_t num_mat;
  GLuint program;
  uint32_t mat_idx;
  char filename[32];

  uint32_t instance_offset; // offset into ssbo of instanced matrices
  uint32_t instances;
  float    instance_mat[48*1024]; // enough for 1024 instances
  uint32_t instance_anim[1024];
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
           program_debug_line, program_terrain,
           program_draw_env, program_debug_flow,
           program_compute_flow;

  uint32_t vao_hud, vbo_hud;
  sx_hud_t hud;

  uint32_t vao_hud_text, vbo_hud_text[3];
  uint32_t hud_text_chars; // number of characters, 100 max
  uint64_t hud_text_font;  // texture handle (bindless)
  float hud_text_vx[2000], hud_text_uv[2000];
  uint32_t hud_text_id[2000];

  uint32_t debug_line_cnt;
  float debug_line_vx[20000];
  uint32_t vao_debug_line, vbo_debug_line;

  uint32_t vao_terrain, vbo_terrain[2];
  uint32_t terrain_vx_cnt, terrain_idx_cnt;
  uint32_t *terrain_idx;
  float    *terrain_vx;

  fbo_t *fbo0, *fbo1; // ping pong fbos for taa
  fbo_t *fbo;         // current pointer
  fbo_t *raster;      // rasterised geo, unwarped
  fbo_t *fsb;         // full screen buffer for terrain and comp
  fbo_t *cube[4];     // cubemap for rasterised geo
  uint32_t cube_side; // what are we currently rendering

  uint32_t num_geo;
  sx_geo_t geo[2048];

  uint32_t  geo_instance_cnt;
  uint32_t  geo_instance_ssbo;

  sx_material_t geo_material[4096];
  uint32_t geo_material_cnt;
  uint32_t geo_material_ssbo;

  uint32_t  geo_anim_ssbo;

  // terrain textures and "character" detail textures:
  uint32_t tex_terrain[8];   // col, dis, mat, ccol, cdis, cmat, acc, cacc

  uint32_t tex_flow_cnt;
  uint32_t tex_flow_curr;
  uint32_t tex_flow[2];

  // TODO: 
  uint32_t tex_skypal, tex_clouds;

  uint64_t env_cloud_noise0;
  uint64_t env_cloud_noise1;
}
sx_vid_t;

void sx_vid_add_debug_line(const float *v0, const float *v1);
void sx_vid_clear_debug_line();
