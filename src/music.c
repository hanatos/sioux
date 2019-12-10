#include <stdint.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "sx.h"
#include "music.h"

int sx_music_init(sx_music_t *m, const char *filename)
{
  memset(m, 0, sizeof(*m));

  m->music = Mix_LoadMUS(filename);

  if(m->music == NULL)
  {
    char fn[1024];
    snprintf(fn, sizeof(fn), "res/%s", filename);
    m->music = Mix_LoadMUS(fn);
  }

  if (m->music == NULL)
  {
    fprintf(stderr, "[music] failed to open music file %s!\n", filename);
    fprintf(stderr, "[music] reason: %s\n", Mix_GetError());
    return 1;
  }

  m->type = Mix_GetMusicType(m->music);
  strcpy(m->filename, filename);
  return 0;
}

void sx_music_cleaunp(sx_music_t *m)
{
  Mix_FreeMusic(m->music);
  free(m);
}

int sx_music_play(sx_music_t *m, int loops)
{
  if(m != NULL)
  {
    if(Mix_PlayingMusic())
      Mix_HaltMusic();
    if(Mix_PlayMusic(m->music, loops) == 0)
    {
      fprintf(stderr, "[music] failed to play music file %s!\n", m->filename);
      fprintf(stderr, "[music] reason: %s\n", Mix_GetError());
      return 1;
    }
    fprintf(stderr, "[music] playing %s\n", m->filename);
    return 0;
  }
  return 1;
}

