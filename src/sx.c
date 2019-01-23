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
  SDL_AudioSpec spec = {0};
  spec.freq = 11025;
  spec.format = AUDIO_U8;
  spec.channels = 1;
  spec.samples = 4096; // buffer size
  spec.callback = 0;
	sx.audio_dev = SDL_OpenAudioDevice(NULL, 0, &spec, &sx.audio_spec, 0);
  // fprintf(stderr, "[sx] got audio spec %d %d %d\n", sx.audio_spec.channels, sx.audio_spec.freq, sx.audio_spec.samples);
  SDL_PauseAudioDevice(sx.audio_dev, 0); // start playing
  return 0;
}

void
sx_cleanup()
{
  SDL_PauseAudioDevice(sx.audio_dev, 1); // stop playing
	SDL_CloseAudioDevice(sx.audio_dev);
  sx_vid_cleanup(); // shuts down SDL
}
