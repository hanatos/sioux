#pragma once

#include <stdint.h>
#include <SDL.h>
#include <SDL_mixer.h>
typedef struct sx_sound_t
{
  char filename[256];
  SDL_AudioSpec wav_spec;
  uint32_t      wav_len;
  uint8_t      *wav_buf;
  Mix_Chunk *chunk;
}
sx_sound_t;

int sx_sound_init(sx_sound_t *s, const char *filename);

void sx_sound_cleaunp(sx_sound_t *s);

int sx_sound_play(sx_sound_t *s, int channel);
int sx_sound_loop(sx_sound_t *s, int channel, int count);
int sx_sound_stop_loop(int channel);
