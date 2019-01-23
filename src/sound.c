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
	SDL_AudioSpec *ret = SDL_LoadWAV(filename, &s->wav_spec, &s->wav_buf, &s->wav_len);
  if(ret == 0)
  {
    char fn[1024];
    snprintf(fn, sizeof(fn), "res/%s", filename);
    ret = SDL_LoadWAV(fn, &s->wav_spec, &s->wav_buf, &s->wav_len);
  }
  if(ret == 0)
  {
    fprintf(stderr, "[sound] failed to open sound file %s!\n", filename);
    fprintf(stderr, "[sound] reason: %s\n", SDL_GetError());
    return 1;
  }
  strcpy(s->filename, filename);
  return 0;
}

void
sx_sound_cleaunp(
    sx_sound_t *s)
{
  // do this after CloseAudioDevice(id)
	SDL_FreeWAV(s->wav_buf);
}

int
sx_sound_play(
    sx_sound_t *s)
{
  fprintf(stderr, "[sound] playing %s\n", s->filename);
	return SDL_QueueAudio(sx.audio_dev, s->wav_buf, s->wav_len);
}

