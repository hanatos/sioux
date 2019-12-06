#include "sx.h"
#include "file.h"
#include "vid.h"
#include "world.h"

#include <stdlib.h>


int main(int argc, char *argv[])
{
  // global init
  sx_init();

  const char *mission = "c1m1.mis";
  if(argc > 1) mission = argv[1];

  const char *filename = mission;
  FILE *f = file_open(filename);
  strncpy(sx.mission.filename, filename, sizeof(sx.mission.filename));
  c3_mission_load(&sx.mission, f);
  fclose(f);

  c3_mission_begin(&sx.mission);
  sx_vid_start_mission();
  uint32_t last_event = SDL_GetTicks();
  uint32_t frames = 0;
  sx.time = last_event;
  uint32_t sim_time = last_event;
  const uint32_t delta_sim_time = 1000.0f/60.0f; // usual vsync
  while(1)
  {
    sx_vid_render_frame();

    uint32_t end = SDL_GetTicks();
    while(sim_time < end)
    {
      if(sx_vid_handle_input()) goto out;
      // TODO:
      // sx_ai_think();
      sx_world_move(delta_sim_time);//end - sx.time);
      sim_time += delta_sim_time;
    }
    // another second passed, run game mechanics only then
    frames++;
    if(end - last_event > 1000)
    {
      c3_mission_pump_events(&sx.mission);
      // if(sx.mission.time > 2) break; // DEBUG
      fprintf(stderr, "\r %.3g ms", (end - last_event)/(double)frames);
      frames = 0;
      last_event = end;
    }
    sx.time = end;
  }
out:
  sx_vid_end_mission();
  c3_mission_end(&sx.mission);
  sx_cleanup(); // alse cleans vid
  exit(0);
}
