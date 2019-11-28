#include "sx.h"
#include "sound.h"

#include <stdint.h>
#include <SDL.h>

int
sx_sound_init(
    sx_sound_t *s,
    const char *filename)
{
  memset(s, 0, sizeof(*s));

  s->wav_spec = sx.audio_spec;
	s->chunk = Mix_LoadWAV(filename);
  if(s->chunk == 0)
  {
    char fn[1024];
    snprintf(fn, sizeof(fn), "res/%s", filename);
    s->chunk = Mix_LoadWAV(fn);
  }

  int channels;
  Mix_QuerySpec(&sx.audio_spec.freq, &sx.audio_spec.format, &channels);
  sx.audio_spec.channels = channels;

  if(s->chunk == 0)
  {
    fprintf(stderr, "[sound] failed to open sound file %s!\n", filename);
    fprintf(stderr, "[sound] reason: %s\n", Mix_GetError());
    return 1;
  }

  strcpy(s->filename, filename);
  return 0;
}

void
sx_sound_cleaunp(
    sx_sound_t *s)
{
	Mix_FreeChunk(s->chunk);
	free(s);
}

int
sx_sound_play(
    sx_sound_t *s)
{
  fprintf(stderr, "[sound] playing %s\n", s->filename);
  Mix_PlayChannel(-1, s->chunk, 0);
  return 0;
}

