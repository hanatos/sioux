#include "c3object.h"
#include "c3model.h"
#include "file.h"
#include "vid.h"

uint32_t c3_object_load(sx_object_t *o, const char *filename)
{
  // load .ai file
  memset(o, 0, sizeof(sx_object_t));
  strncpy(o->filename, filename, sizeof(o->filename)-1);
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
    if(o->geoid[o->num_geo] != -1)
    {
      uint64_t len;
      c3m_header_t *h = file_load(line, &len);
      o->geo_off_cnt[o->num_geo] = c3m_get_child_offsets(h, o->geo_off + o->num_geo);
    }
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
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%4[^,],%d", o->draw, &o->geo_h);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%4[^,],%d", o->dram, &o->geo_m);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%4[^,],%d", o->dral, &o->geo_l);
  // read movement
  if(file_readline(f, line)) goto fail;
  memcpy(o->move, line, 4);
  // need to skip to '<' because there is an optional block:
  // 4       ;rate
  // 1       ;control registers *
  // 4       ;number of frames
  // (see fire.ai, i think we handle this by animated textures already)
  // read extra flags, hitpoints, printable name for radar?
  // <
  // 1       ;Collidable
  // 0       ;Missile / Projectile
  // 0       ;Landable - You can land on the top of this object
  // 
  // 1       ;Good Team - Bad Target
  // 0       ;Bad Team - Good Target
  // 1       ;High Priority Target - Friendly Fire System
  // 1       ;Air
  // 
  // 20      ;hit points
  // 3       ;(0=UNK,1=TaNK,2=AIR,3=COPter,4=GUN,5=MiSsiLe)
  // RAH-66
  // <
  while(1)
  {
    if(file_readline(f, line)) goto fail;
    if(line[0] == '<') break; // found '<'
  }
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->collidable);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->projectile);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->landable);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->good_team_bad_target);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->bad_team_good_target);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->high_priority_target);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->goal);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%g", &o->hitpoints);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%d", &o->type);
  if(file_readline(f, line)) goto fail;
  sscanf(line, "%s", o->description);

  fclose(f);
  return 0;
fail:
  fclose(f);
  return 1;
}
