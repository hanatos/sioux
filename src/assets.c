#include "assets.h"
#include "c3object.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>

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
    if(!strcmp(a->music[k].filename, filename))
      return k;
  assert(a->num_music < sizeof(a->music)/sizeof(sx_music_t));
  if(sx_music_init(a->music+a->num_music, filename)) return -1;
  return a->num_music++;
}

uint32_t
sx_assets_load_object(
    sx_assets_t *a,
    const char *filename)
{
  // in fact, we need to keep duplicates (or put references)
  // because the .pos files will refer to this index:
  // stupid dedup:
  // for(int k=0;k<a->num_objects;k++)
  //   if(!strcmp(a->object[k].filename, filename))
  //     return k;
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
