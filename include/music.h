#pragma once

#include <stdint.h>
#include <SDL.h>
#include <SDL_mixer.h>

typedef struct sx_music_t
{
  char filename[256];
  Mix_Music* music;
  Mix_MusicType type;
}
sx_music_t;

int sx_music_init(sx_music_t *m, const char *filename);

void sx_music_cleaunp(sx_music_t *m);

int sx_music_play(sx_music_t *m, int loops);

