#include "assets.h"
#include "c3object.h"
#include "triggers.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// returns handle and dedupes
// please only pass lower case strings
uint32_t
sx_assets_load_sound(
    sx_assets_t *a,
    const char *filename)
{
  // stupid:
  for(int k=0;k<a->num_sounds;k++)
    if(!strcmp(a->sound[k].filename, filename))
      return k;
  assert(a->num_sounds < sizeof(a->sound)/sizeof(sx_sound_t));
  if(sx_sound_init(a->sound+a->num_sounds, filename)) return -1;
  return a->num_sounds++;
}

uint32_t
sx_assets_load_music(
    sx_assets_t *a,
    const char *filename)
{
  for(int k=0;k<a->num_music;k++)
    if(!strcmp(a->fmusic[k]->filename, filename))
      return k;
  assert(a->num_music < C3_GAMESTATE_SIZE);
  if(sx_music_init(sx_assets_filename_to_music(a, filename), filename))
		  return -1;
  return a->num_music++;
}

sx_music_t* sx_assets_filename_to_music(sx_assets_t* a, const char* filename)
{
  // find gamestate i corresponding to filename
  int i = C3_GAMESTATE_FLIGHT;
  if(!strncmp(filename+1, "win", 3)) i = C3_GAMESTATE_WIN;
  else if(!strncmp(filename+1, "lose", 4)) i = C3_GAMESTATE_WIN;
  else if(!strncmp(filename+2, "lwin", 3)) i = C3_GAMESTATE_WIN;
  else if(!strncmp(filename+2, "dire", 4)) i = C3_GAMESTATE_DIRE;
  else if(!strncmp(filename+2, "flight", 6)) i = C3_GAMESTATE_FLIGHT;
  else if(!strncmp(filename+2, "lwin", 4)) i = C3_GAMESTATE_LOSE_WIN;
  else if(!strncmp(filename+2, "combat", 6)) i = C3_GAMESTATE_COMBAT;
  else if(!strncmp(filename+2, "pad", 3)) i = C3_GAMESTATE_PAD;
  else if(!strncmp(filename+2, "scary", 3)) i = C3_GAMESTATE_SCARY;

  if(filename[0] == 'f')
    return a->fmusic[i];
  return a->gmusic[i];
}

uint32_t
sx_assets_load_object(
    sx_assets_t *a,
    const char *filename,
    int dedup)
{
  // in fact, we need to keep duplicates (or put references)
  // because the .pos files will refer to this index. after loading
  // has finished, we might get a previously loaded object (explosion)
  // from the list by exhaustive search:
  if(dedup) // stupid dedup:
    for(int k=0;k<a->num_objects;k++)
      if(!strcmp(a->object[k].filename, filename))
        return k;
  assert(a->num_objects < sizeof(a->object)/sizeof(sx_object_t));
  // fprintf(stderr, "[assets] loading object %u %s\n", a->num_objects, filename);
  if(c3_object_load(a->object+a->num_objects, filename))
  {
    fprintf(stderr, "[assets] failed to load `%s'!\n", filename);
    return -1;
  }
  return a->num_objects++;
}

// TODO: routine to nuke the whole thing and start from scratch
