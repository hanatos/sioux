#include "c3mission.h"
#include "c3pos.h"
// #include "c3jim.h"
#include "triggers.h"
#include "camera.h"
#include "sx.h"
#include "physics/heli.h"

// TODO: need these return values?
int
c3_mission_begin(
    c3_mission_t *mis)
{
  uint32_t objectid = 0;
  uint32_t startposid = 0;
  quat_t q;
  quat_init_angle(&q, 0, 0, 0, 1);

  char posname[256];
  strncpy(posname, mis->filename, sizeof(posname));
  char *c = posname + strlen(posname);
  while(*c != '.' && c > posname) c--;
  c[1] = 'p'; c[2] = 'o'; c[3] = 's'; c[4] = 0;

  // does not seem to contain anything useful on top of what's in the mis file already:
  // char jimname[256];
  // strncpy(jimname, mis->filename, sizeof(jimname));
  // c = jimname + strlen(jimname);
  // while(*c != '.' && c > jimname) c--;
  // c[1] = 'j'; c[2] = 'i'; c[3] = 'm'; c[4] = 0;
  // c3_jim_load(jimname);

  uint64_t len = 0;
  c3_pos_t *f = file_load(posname, &len);
  c3_pos_t *orig = f;
  uint64_t cnt = (len - 10*4)/ sizeof(c3_pos_t);
  // double cntd = (len - 10*4 )/ (double)sizeof(c3_pos_t);
  // fprintf(stderr, "[mission] got %g structs\n", cntd);

  int block = 0;
  for(int i=0;i<cnt;i++)
  {
    uint32_t *marker = (uint32_t *)(f+i);
    while(marker[0] == -1)
    {
      block++;
      // skip end marker
      f = (c3_pos_t *)(((uint8_t *)f)+4);
      marker = (uint32_t *)(f+i);
    }

    // convert from yards with one binary decimal to feet
    float pos[3] = {-ft2m(f[i].x*3.0f/4.0f), 0.0f, -ft2m(f[i].y*3.0f/4.0f)};
    const int fobj = c3_pos_objectid(f+i);
    if(fobj >= 0 && fobj < sx.assets.num_objects)
      objectid = fobj;
    // read orientation
    const float heading = 2.0f*M_PI*f[i].heading/(float)0xffff;
    quat_from_euler(&q, 0, 0, heading);
    uint32_t eid = sx_world_add_entity(objectid, pos, &q, 'A'+c3_pos_groupid(f+i), c3_pos_campid(f+i));
#if 0 // XXX DEBUG
    if(strcmp(sx.assets.object[objectid].filename, "oak4"))
    {
      fprintf(stderr, "%s ent %u at %g %g %g obj %u | '%c' %d\n",
          sx.assets.object[objectid].filename,
          eid, pos[0], pos[1], pos[2], objectid,
          'A' + c3_pos_groupid(f+i), c3_pos_groupid(f+i));
      fprintf(stderr, "extra %u %u %u %d %u %u %u %u\n", f[i].zero0,
          f[i].zero1, f[i].zero2, f[i].what0, f[i].flags[0], f[i].flags[1],
          f[i].flags[2], f[i].flags[3]);
      fprintf(stderr, "block %d\n", block);
    }
#endif
    // found start position:
    if(!strcmp(sx.assets.object[objectid].filename, "startpos"))
    {
      // fprintf(stderr, "startpos entity %u at %g %g %g tile %d %d\n", //with extra %hX %hX %d %d\n",
      //     eid, pos[0], pos[1], pos[2], f[i].tilex, f[i].tiley);
      startposid = eid;
    }
  }

  const uint32_t player_objectid = 0; // XXX get this while reading the assets!
  float *pos = sx.world.entity[startposid].body.c;
  q = sx.world.entity[startposid].body.q;
  // add player entity and attach camera and movement controller:
  uint32_t eid = sx_world_add_entity(player_objectid, pos, &q,
      sx.world.entity[startposid].id, sx.world.entity[startposid].camp);

  // setup start position:
  sx_camera_init(&sx.cam);
  sx.cam.hfov = 2.64;
  sx.cam.vfov = sx.cam.hfov * sx.height/(float)sx.width;
  float off[3] = {20.0f, 20.0f, 20.0f};
  sx_camera_target(&sx.cam, pos, &q, off, 1);
  sx_camera_move(&sx.cam, 1);

  // add movement controller:
  sx.world.player_entity = eid;

  // setup helicopter movement! yay!
  sx.world.player_move = malloc(sizeof(sx_heli_t));
  sx_heli_init(sx.world.player_move, sx.world.entity + eid);
  sx.world.entity[eid].move_data = sx.world.player_move; // TODO: this is the only one we should need
  sx.world.entity[eid].move.update_forces = &sx_heli_update_forces;
  sx.world.entity[eid].move.damage = &sx_heli_damage; // TODO: pending API updates

  free(orig);
  return 0;
}

int
c3_mission_end(
    c3_mission_t *mis)
{
  return 0;
}

void
c3_mission_pump_events(
    c3_mission_t *mis)
{
  const float *player_pos = sx.world.entity[sx.world.player_entity].body.c;
  float wp[] = {
    sx.mission.waypoint[0][sx.world.player_wp][0] - player_pos[0],
    0.0f,
    sx.mission.waypoint[0][sx.world.player_wp][1] - player_pos[2]};
  if(dot(wp, wp) < 100.0f*100.0f)
    if(sx.world.player_wp < 10) sx.world.player_wp++;

  c3_triggers_check(mis);
  mis->time ++;
  if(mis->counter > 0)
    mis->counter ++;
}

int
c3_mission_load(
  c3_mission_t *mis,
  FILE *f)
{
  char line[1024];
  // [green, desert, snow]
  if(file_readline(f, line)) return 1;
  mis->envtype = 'g';
  if     (!strncmp(line, "green",  5)) mis->envtype = 'g';
  else if(!strncmp(line, "desert", 6)) mis->envtype = 'd';
  else if(!strncmp(line, "snow",   4)) mis->envtype = 's';

  // terrain textures
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->colourmap);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->heightmap);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->detailmap);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->cdis);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->cmat);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->ccol);

  if(sx_vid_init_terrain(
        mis->colourmap, mis->heightmap, mis->detailmap,
        mis->ccol, mis->cdis, mis->cmat))
    return 1;

  // sky environment
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->clouds);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->skypal);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->sun);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%s", mis->stars);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%g", &mis->sun_slope);

  // terrain parameters
  if(file_readline(f, line)) return 1;
  sscanf(line, "%u", &mis->elevation);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%u", &mis->cloud_height);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%u", &mis->terrain_scale_bits);

  if(sx_world_init_terrain(mis->heightmap,
        mis->elevation, mis->cloud_height, mis->terrain_scale_bits))
    return 1;

  if(file_readline(f, line)) return 1;
  sscanf(line, "%hhu %hhu", &mis->gamma, &mis->saturation);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%hhu %hhu %hhu", mis->rgb+0, mis->rgb+1, mis->rgb+2);

  // heli configuration
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->cannon);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->rockets);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->stingers);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->hellfire);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->artillery);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->wingmen);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->sidewinders);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->lineofsight);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->show_efams);

  if(file_readline(f, line)) return 1;
  mis->music = line[0];
  char filename[16];
  for(int i = 0; i != (C3_COND_SCARY - C3_COND_DIRE + 1); i++)
  {
     c3_triggers_parse_music(filename, mis->music, i ,'f');
     sx_assets_load_music(&sx.assets, filename);
  }
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->mission_type);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->copilot);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->ideal_altitude);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->magic_pal_number);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%d", &mis->weight);

  if(file_readline(f, line)) return 1;
  sscanf(line, "%*d"); // future use 0
  if(file_readline(f, line)) return 1;
  sscanf(line, "%*d"); // future use 1

  if(file_readline(f, line)) return 1;
  if(line[0] != '<') fprintf(stderr, "expected '<'!\n");

  // ai file section:
  // one ai file per line (without .ai suffix, needs tolower())
  // parse these, load dependent 3do and load dependent textures
  while(!feof(f))
  {
    if(file_readline(f, line)) return 1;
    if(line[0] == '<') break; // end marker reached
    // returns object handle
    if(sx_assets_load_object(&sx.assets, line) == -1) return 1;
  }

  // wind data
  while(!feof(f))
  {
    if(file_readline(f, line)) return 1;
    if(line[0] == '<') break; // last line is end marker '<'
    uint32_t start, stop, heading, speed;
    sscanf(line, "W,%d,%d,%d,%d", &start, &stop, &heading, &speed);
  }

  // waypoint data in blocks for waypoints A-P:
  // initial name is either optional or only exists for A
  int wp_list = 0; // 'A' .. 'P' is 0..something
  int wp = 0;      // wp_list = 0, wp = 0 will be "A1"
  while(!feof(f))
  {
    if(file_readline(f, line)) return 1;
    if(line[0] == '<')
    {
      // all waypoints finished?
      if(line[1] == '<') break;
      // next waypoint list
      wp_list++;
      wp = 0;
      continue;
    }

    int x, y;         // 0-8191
    char name[30];    // i think 15+0 is max
    if(wp_list == 0)  // 'A' list has names
    {
      sscanf(line, "%[^,], %d, %d", name, &x, &y);
    }
    else
      sscanf(line, "%d, %d", &x, &y);

    // TODO: maybe the *2 is a wrong bit count when reading the pos file?
    mis->waypoint[wp_list][wp][0] = -ft2m(3.0f*x*2.0f);
    mis->waypoint[wp_list][wp][1] = -ft2m(3.0f*y*2.0f);
    // if(wp_list == 0) fprintf(stderr, "%s ", name);
    // fprintf(stderr, "wp %c%d %g %g\n", 'A' + wp_list, wp+1, 8.0f*x, -8.0f*y);
    wp++;
  }
  sx.world.player_wp = 0;
  sx.world.player_weapon = 0;

  mis->time = 0;
  mis->counter = 0;

  // XXX TODO: we still need to load .ord .inf for briefing

  // it follows: triggers (see triggers.h)
  return c3_triggers_parse(mis, f);
}
