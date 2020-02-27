#pragma once

// TODO: include submodule header?
#include "vid/gl/vid.h"
#include "quat.h"

#include <SDL.h>

int sx_vid_init(uint32_t wd, uint32_t ht, int fullscreen);

void sx_vid_cleanup();

// memory allocation:
void sx_vid_start_mission();
void sx_vid_end_mission();

// init renderable geo, return handle
// if aabb is given, will be filled with geo aabb
uint32_t sx_vid_init_geo(const char *filename, float *aabb);

// load terrain textures
int sx_vid_init_terrain(
    const char *col, const char *hgt, const char *mat,
    const char *ccol, const char *chgt, const char *cmat);

void sx_vid_render_frame();
void sx_vid_render_frame_rect();
void sx_vid_push_geo_instance(const uint32_t gi,
    const float *omvx, const quat_t *omvq,
    const float *mvx,  const quat_t *mvq,
    uint32_t frame);

// return if exit?
int sx_vid_handle_input();

// returns buffer position after added text, bufpos is passed and memory will be
// overwritten there. there's a limited amount of space on the hud for text
uint32_t sx_vid_hud_text(const char *text, uint32_t bufpos, float x, float y, int center_x, int center_y);
void sx_vid_hud_text_clear();

typedef struct sx_camera_t sx_camera_t;
void sx_vid_compute_mvp(
    float        *MVP,
    float        *MV,
    const int     u_cube_side,
    const quat_t *q_model,   // orientation of model (model to world quaternion)
    const float  *p_model,   // world space position of model coordinate system
    sx_camera_t  *cam,
    int           old,       // use prev cam coordinates?
    int c3model);            // 1 if the model is a c3model (introduces more axis flips)
