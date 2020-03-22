#pragma once

#include "sound.h"
#include "music.h"
#include "c3object.h"
#include "triggers.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>


typedef struct sx_assets_t
{
  sx_sound_t sound[512];
  uint32_t num_sounds;

  sx_music_t fmusic[8][8];
  sx_music_t gmusic[8][8];
  sx_music_t* cur_music;

  uint32_t num_music;

  sx_object_t object[512];  // these are the unique objects
  uint32_t num_objects;     // entities in world struct make use of these
}
sx_assets_t;

// returns handle and dedupes
// please only pass lower case strings
uint32_t sx_assets_load_sound(
    sx_assets_t *a,
    const char *filename);

uint32_t sx_assets_load_music(
    sx_assets_t *a,
    char variant, char gf);

sx_music_t* sx_assets_filename_to_music(
    sx_assets_t* a,
    const char* filename);

// returns handle, does not dedup unless instructed (need to keep index for pos files)
// use lower case file names
// loads .ai files
uint32_t sx_assets_load_object(
    sx_assets_t *a,
    const char *filename,
    int dedup);
// TODO: routine to nuke the whole thing and start from scratch
