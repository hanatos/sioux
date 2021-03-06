#pragma once

#include "triggers.h"
#include "file.h"

// TODO: load .ord -> mission briefing text + background image
// TODO: load .inf -> radio messages

// TODO: load .pos // XXX binary! with 28-byte strides, seems to jump right in
// TODO: (load .fnt files?)

typedef struct c3_mission_t
{
  char filename[256];  // original mis file name
  char envtype;        // 'g' 'd' 's'
  char colourmap[256]; // .pcx
  char heightmap[256];
  char detailmap[256];
  char cdis[256];
  char cmat[256];
  char ccol[256];

  char clouds[256];    // .pcx cloud layer
  char skypal[256];    // .pcx sky gradient, 4x4 image
  
  char  sun[256];      // .3do
  char  stars[256];    // .sky
  float sun_slope;     // angle in degrees

  // all in feet :/
  uint32_t elevation;          // elevation at heightmap == 0
  uint32_t cloud_height;
  uint32_t terrain_scale_bits; // (19=192ft, 20=384ft, 21=768ft)
  uint8_t gamma, saturation;
  uint8_t rgb[3];

  // start configuration:
  int cannon;
  int rockets;
  int stingers;
  int hellfire;
  int artillery;
  int wingmen;
  int sidewinders;
  int lineofsight;
  int show_efams;

  // music to play:
  char music;           // single letter 'A'..'D' or '0'? might come from cdda
  int mission_type;     // 0 coop 1 melee
  int copilot;
  int ideal_altitude;   // suppose in feet over ground? or over elevation?
  int magic_pal_number;
  int weight;           // of the helicopter in pounds???

  // radio message texts
  char radiomessage[100][256];

  // run time stuff for triggers:
  uint32_t num_triggers;
  c3_trigger_t trigger[1000];
  uint32_t counter;
  uint32_t time;
  c3_gamestate_t gamestate;

  // global sounds for the hero comanche:
  uint32_t snd_engine;
  uint32_t snd_warn_speed;
  uint32_t snd_warn_lock;
  uint32_t snd_warn_torque;
  uint32_t snd_cannon;

  uint32_t snd_fire;
  uint32_t snd_hit;
  uint32_t snd_explode;
  uint32_t snd_scrape;

  // global dynamic obj for dynamic spawning
  uint32_t obj_fire;
  uint32_t obj_explosion_air;
  uint32_t obj_explosion_dirt;
  uint32_t obj_explosion_nuke;
  uint32_t obj_debris;
  uint32_t obj_rocket;
  uint32_t obj_stinger;
  uint32_t obj_hellfire;
  uint32_t obj_bullet;
  uint32_t obj_trail;
  uint32_t obj_dead_coma;
  uint32_t obj_dead_copt;
}
c3_mission_t;

// load .mis file
int c3_mission_load(c3_mission_t *mis, FILE *f);

int c3_mission_begin(c3_mission_t *mis);

int c3_mission_end(c3_mission_t *mis);

void c3_mission_pump_events(c3_mission_t *mis);
