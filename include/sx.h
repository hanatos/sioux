#pragma once

#include "assets.h"
#include "c3mission.h"
#include "vid.h"
#include "world.h"
#include "camera.h"

typedef struct sx_t
{
  sx_assets_t  assets;  // cache of shared assets
  c3_mission_t mission; // currently active mission
  sx_world_t   world;   // current state of world with list of entities

  SDL_AudioDeviceID audio_dev;
  SDL_AudioSpec     audio_spec;

  sx_vid_t vid;
  sx_camera_t cam;
  
  uint32_t width, height;

  uint32_t time;         // time in milliseconds
  uint32_t paused;       // 1 if paused, 0 otherwise
  float    lod;          // level of detail
}
sx_t;

extern sx_t sx;

int sx_init(int argc, char *argv[]);

void sx_cleanup();
