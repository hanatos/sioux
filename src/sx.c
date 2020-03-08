#include "sx.h"
#include "vid.h"

#include <string.h>

sx_t sx; // linkage

int
sx_init(int argc, char *argv[])
{
  memset(&sx, 0, sizeof(sx));
  sx.width  = 1280;
  sx.height = 640;
  sx.width  = 1920;
  sx.height = 1080;
  // sx.width  = 3840;
  // sx.height = 2160;
  sx.lod = 4;

  int fullscreen = 0;
  for(int i=0;i<argc;i++)
  {
    if     (!strcmp(argv[i], "--fullscreen")) fullscreen = 1;
    else if(!strcmp(argv[i], "--lod") && i+1 < argc) sx.lod = atof(argv[++i]);
  }

  sx_vid_init(sx.width, sx.height, fullscreen); // also inits SDL

	// open audio device
  sx.audio_spec.freq = 11025;
  sx.audio_spec.format = AUDIO_U8;
  sx.audio_spec.channels = 1;
  sx.audio_spec.samples = 128;//4096; // buffer size
  sx.audio_spec.callback = 0;
  sx.audio_dev = Mix_OpenAudio(sx.audio_spec.freq, sx.audio_spec.format, sx.audio_spec.channels, sx.audio_spec.samples);
  if ( Mix_AllocateChannels(8) < 0)
  {
    fprintf(stderr, "[sx] unable to allocate mixing channels: %s\n", Mix_GetError());
    exit(EXIT_FAILURE);
  }
  Mix_ReserveChannels(1); // for radio
  fprintf(stderr, "[sx] got audio spec %d %d %d\n", sx.audio_spec.channels, sx.audio_spec.freq, sx.audio_spec.samples);
  sx_world_init();
  return 0;
}

void
sx_cleanup()
{
  Mix_CloseAudio();
  sx_vid_cleanup(); // shuts down SDL
}
