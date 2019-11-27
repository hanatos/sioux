#include "sx.h"
#include "vid.h"

sx_t sx; // linkage

int
sx_init()
{
  sx.width = 1280;
  sx.height = 640;
  sx.width = 1920;
  sx.height = 1080;

  sx_vid_init(sx.width, sx.height); // also inits SDL

	// open audio device
  sx.audio_spec.freq = 11025;
  sx.audio_spec.format = AUDIO_U8;
  sx.audio_spec.channels = 1;
  sx.audio_spec.samples = 4096; // buffer size
  sx.audio_spec.callback = 0;
  sx.audio_dev = Mix_OpenAudio(sx.audio_spec.freq, sx.audio_spec.format, sx.audio_spec.channels, sx.audio_spec.samples);
  if ( Mix_AllocateChannels(8) < 0)
  {
    fprintf(stderr, "Unable to allocate mixing channels: %s\n", Mix_GetError());
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "[sx] got audio spec %d %d %d\n", sx.audio_spec.channels, sx.audio_spec.freq, sx.audio_spec.samples);
  return 0;
}

void
sx_cleanup()
{
  Mix_CloseAudio();
  sx_vid_cleanup(); // shuts down SDL
}
