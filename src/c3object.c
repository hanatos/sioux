#include "c3object.h"
#include "file.h"
#include "vid.h"

uint32_t c3_object_load(sx_object_t *o, const char *filename)
{
  // load .ai file
  memset(o, 0, sizeof(sx_object_t));
  strncpy(o->filename, filename, sizeof(o->filename));
  char file[256];
  snprintf(file, sizeof(file), "%s.ai", filename);

  FILE *f = file_open(file);
  if(!f)
  {
    fprintf(stderr, "[c3o] failed to load `%s'\n", filename);
    return 1;
  }

  char line[1024];
  while(!feof(f))
  {
    // read next non-empty line, ignore ; comments
    if(file_readline(f, line)) return 1;
    if(line[0] == '>')
    { // desert green or snow offsets?
      if(line[1] == 'd') o->geo_d = o->num_geo;
      if(line[1] == 's') o->geo_s = o->num_geo;
      if(line[1] == 'g') o->geo_g = o->num_geo;
      continue;
    }
    if(line[0] == '<')
    {
      if(line[1] == '<') break;
      continue;
    }
    // upload 3do to vid, remember id
    for(int k=0;k<sizeof(line);k++)
      if(line[k] == ' ' || line[k] == '\r' || line[k] == '\n')
      { line[k] = 0; break; }
    o->geoid[o->num_geo] = sx_vid_init_geo(line, o->geo_aabb[o->num_geo]);
    o->num_geo++;
  }
  // compute parent bounding box:
  o->aabb[0] = o->aabb[1] = o->aabb[2] =  FLT_MAX;
  o->aabb[3] = o->aabb[4] = o->aabb[5] = -FLT_MAX;
  for(int k=0;k<o->num_geo;k++)
  {
    // fprintf(stderr, "geo %d bounding box: %g %g %g %g %g %g\n", k,
    //   o->geo_aabb[k][0], o->geo_aabb[k][1], o->geo_aabb[k][2],
    //   o->geo_aabb[k][3], o->geo_aabb[k][4], o->geo_aabb[k][5]);
    for(int i=0;i<3;i++)
    {
      o->aabb[  i] = MIN(o->aabb[  i], o->geo_aabb[k][  i]);
      o->aabb[3+i] = MAX(o->aabb[3+i], o->geo_aabb[k][3+i]);
    }
  }
  // fprintf(stderr, "bounding box: %g %g %g %g %g %g\n",
  //     o->aabb[0], o->aabb[1], o->aabb[2],
  //     o->aabb[3], o->aabb[4], o->aabb[5]);
  // read plot routines and geo offset, ignore fading distance
  if(file_readline(f, line)) return 1;
  sscanf(line, "%4[^,],%d", o->draw, &o->geo_h);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%4[^,],%d", o->dram, &o->geo_m);
  if(file_readline(f, line)) return 1;
  sscanf(line, "%4[^,],%d", o->dral, &o->geo_l);
  // read movement
  if(file_readline(f, line)) return 1;
  strncpy(o->move, line, 4);
  // <
  // read extra flags, hitpoints, printable name for radar?
  // <
  fclose(f);
  return 0;
}
