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
	if(a == NULL || filename == NULL)
		return NULL;

	char* fn = NULL;
	sx_music_t* m = NULL;

	for(int i=0;i<C3_GAMESTATE_SIZE;i++) 
	{
		fn = NULL;
  		if((fn = strstr(filename, c3_condition_midi_text[i])))
        	{
			// 'f'/'g' part of filename
			if(isalpha(filename[0]) != 0 && (filename[0] == 'f' || filename[0] == 'g'))
			{
				// prefix letter part of filename 'a' to 'e' or 'n'
				if(fn - &filename[1] > 0)
				{
					if(fn - &filename[1] ==  1 && (isalpha(filename[1]) != 0 && 
					 ((filename[1] >= 'a' && filename[1] <= 'e') || filename[1] == 'n')))
					{
						if('e' - filename[1] >= 0)
							m = (filename[0] == 'f') ? &a->fmusic[i][6 - ('e' - filename[1] + 1)] 
									       : &a->gmusic[i][6 - ('e' - filename[1] + 1)];
						if(filename[1] == 'n')
							m = (filename[0] == 'f') ? &a->fmusic[i][7] : &a->gmusic[i][7];
						break;
					}
					else
					{
						m = NULL;
						fprintf(stderr, "wrong midi filename format: %s\n", filename);
						break;
					}
				}
				// no letter prefix
				m = filename[0] == 'f' ? &a->fmusic[i][0]
				       		       : &a->gmusic[i][0];
			}
			else
				m = NULL;
		}
		m = NULL;
	}
	return m;
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
