#include "triggers.h"
#include "assets.h"
#include "sx.h"

#define triggers_printf fprintf
// #define triggers_printf(...)

static uint32_t
c3_triggers_identifier_to_int(
    char *id) // will put this to lower case
{
  if(id[0] == 0) return 0;
  const int len = strlen(id);
  if(len <= 2 && isalpha(id[0]))
  {
    // two-char waypoint J6 or J: to two-chars in lower bytes?
    // single char player id P or similar to lower byte?
    return id[0] | (id[1] << 8);
  }
  for(int k=0;k<len;k++) id[k] = tolower(id[k]);
  if(len > 4 && !strncmp(id+len-4, ".wav", 4))
  {
    return sx_assets_load_sound(&sx.assets, id);
  }

  return atol(id); // parse integer number for everything else
}

static void
c3_triggers_int_to_pos(uint32_t id, float *pos)
{
  pos[0] = pos[1] = pos[2] = 0.0f;
  if(id > 0xff)
  { // two char identifier
    char c0 = id&0xff, c1 = id >> 8;
    // W might be waypoint of second byte, i.e. WL waypoint of hind in c1m3
    // but there's also use of WO as the location of some tanks 'O', they don't have waypoints.
    // without further ideas, let's use the coordinate of an alive or dead object of that group.
    // prefer life objects.
    if(c0 == 'W')
    { // coordinates of some object
      int gid = c1 - 'A';
      for(int m=0;m<sx.world.group[gid].num_members;m++)
      {
        for(int k=0;k<3;k++)
          pos[k] = sx.world.group[gid].member[m]->body.c[k];
        if(sx.world.group[gid].member[m]->hitpoints > 0) return;
      }
      return;
    }
    // regular waypoint id, such as A1 or J3
    int wp = c0-'A', wpn = c1-'1';
    if(wp < 20 && wpn <= 10)
    {
      //fprintf(stderr, "computing distance to waypoint %c%c\n", id&0xff, id>>8);
      pos[0] = sx.world.group[wp].waypoint[wpn][0];
      pos[1] = 0.0f;
      pos[2] = sx.world.group[wp].waypoint[wpn][1];
    }
  }
}

int
c3_triggers_parse(c3_mission_t *mis, FILE *f)
{
  char op[256], arg0[256], arg1[256];
  char line[256];
  c3_trigger_t *t = mis->trigger;
  int i = 0; // trigger number
  int c = 0; // condition number
  int expect = 0; // 0 if, 1 cond, 2 op or then, 3 action
  while(!feof(f))
  {
    op[0] = arg0[0] = arg1[0] = 0;
    fscanf(f, "%[^\r\n]\r\n", line);
    // comment handling, half copied from c3mission loading :/
    if(line[0] == ';') continue; // poorest parsing ever
    // ignore ; comments and force lower case
    uint64_t len = strlen(line);
    for(int k=0;k<len;k++)
    {
      if(line[k] == ';')
      {
        line[k] = 0;
        break;
      }
    }
    sscanf(line, "%[^,\r\n], %[^,\r\n], %s\r\n", op, arg0, arg1);

    // fprintf(stderr, "op %s arg %s arg %s\n", op, arg0, arg1);

    int a0 = c3_triggers_identifier_to_int(arg0);
    int a1 = c3_triggers_identifier_to_int(arg1);

    if(expect == 0)
    {
      if(op[0] == '<') break; // end of block
      if(!strcmp(op, "if"))
      {
        c = 0;
        t[i].oper[0] = C3_OP_AND;
        expect = 1;
      }
    }
    else if(expect == 1)
    { // condition
      t[i].cond[c] = C3_COND_CNT;
      t[i].arg0[c] = a0;
      t[i].arg1[c] = a1;
      for(int k=0;k<C3_COND_CNT;k++) if(!strcmp(op, c3_condition_text[k]))
      {
        t[i].cond[c] = k;
        break;
      }
      c++; // next condition, if any
      expect = 2;
    }
    else if(expect == 2)
    { // op or then
      t[i].oper[c] = C3_OP_CNT;
      if(!strcmp(op, "then")) expect = 3;
      else
      {
        for(int k=0;k<C3_OP_CNT;k++) if(!strcmp(op, c3_operator_text[k]))
        {
          t[i].oper[c] = k;
          break;
        }
        expect = 1;
      }
    }
    else if(expect == 3)
    { // action
      t[i].action = C3_ACT_CNT;
      t[i].aarg0 = a0;
      t[i].aarg1 = a1;
      t[i].enabled = 1;
      for(int a=0;a<C3_ACT_CNT;a++)
      {
        if(!strcmp(op, c3_action_text[a]))
        {
          t[i].action = a;
          break;
        }
      }
      i++; // next trigger
      expect = 0;
    }
  }
  mis->num_triggers = i;
  return i;
}

void
c3_triggers_action(
    c3_mission_t *mis,
    c3_action_t   a,
    uint32_t      arg0,
    uint32_t      arg1)
{
  // TODO
  switch(a)
  {
    case C3_ACT_FLASH:
      sx_hud_flash(&sx.vid.hud, arg0, arg1);
      triggers_printf(stderr, "flashing control %u for %u seconds\n", arg0, arg1);
      break;        // flash, <id>, <times>  // tutorial: flash HUD element
    case C3_ACT_PLAY: // play <wav filename>
      triggers_printf(stderr, "action play sound %u\n", arg0);
      if(arg0 < 0 || arg0 > sx.assets.num_sounds) return;
      sx_sound_play(sx.assets.sound+arg0, 0);
      break;
    case C3_ACT_VAPORIZE:     // vaporize, <object> ???
      break;
    case C3_ACT_TEXT:
      triggers_printf(stderr, "action display text %u\n\"\"\"%s\"\"\"\n", arg0, mis->radiomessage[arg0]);
      sx.world.status_message = mis->radiomessage[arg0];
      break;
    case C3_ACT_ROUTE:
      triggers_printf(stderr, "action route %c %c%u\n", arg0, arg1>>8, (arg1&0xff) + 1);
      for(int m=0;m<sx.world.group[arg0-'A'].num_members;m++)
      {
        int wpg = (arg1>>8);
        int wpn = (arg1&0xff)-1;
        sx.world.group[arg0-'A'].member[m]->curr_wpg = wpg;
        sx.world.group[arg0-'A'].member[m]->curr_wp  = wpn;
      }
      break;
    case C3_ACT_WIN:          // mission won
      sx.mission.gamestate = C3_GAMESTATE_WIN;
      break;
    case C3_ACT_CLEARCOUNTER: // restart from 1
      triggers_printf(stderr, "resetting counter\n");
      mis->counter = 0;
      break;
    default: // nothing happens
      break;
  }
}

int
c3_triggers_check_cond(
    c3_mission_t *mis,
    c3_condition_t c,
    uint32_t arg0,
    uint32_t arg1)
{
  sx_entity_t *P = sx.world.entity+sx.world.player_entity;
  // TODO: big switch over all cases
  switch(c)
  {
    case C3_COND_NEARER:     // nearer, <distance>, <object/waypoint>
      {
        float pos[3];
        c3_triggers_int_to_pos(arg1, pos);
        for(int k=0;k<3;k++)
          pos[k] -= P->body.c[k];
        // triggers_printf(stderr, "distance to waypoint %c%c : %g\n", arg1&0xff, arg1>>8, sqrtf(dot(pos,pos)/2.0f));
        return sqrtf(dot(pos,pos)) < 1.5*ft2m(arg0); // or maybe it was in yards?
      }
    case C3_COND_FARTHER:    // farther, <distance>, <object>
      {
        float pos[3];
        c3_triggers_int_to_pos(arg1, pos);
        for(int k=0;k<3;k++)
          pos[k] -= P->body.c[k];
        // triggers_printf(stderr, "distance to waypoint %c%c : %g\n", arg1&0xff, arg1>>8, sqrtf(dot(pos,pos)/2.0f));
        return sqrtf(dot(pos,pos)) > 1.5*ft2m(arg0);
      }
    case C3_COND_INTACT:      // intact, <object>
      if(arg0 == 'P') return P->hitpoints > 0.0;
      for(int m=0;m<sx.world.group[arg0-'A'].num_members;m++)
        if(sx.world.group[arg0-'A'].member[m]->hitpoints <= 0) return 0;
      return 1;
    case C3_COND_ALIVE:      // alive, <object>
      if(arg0 == 'P') return P->hitpoints > 0.0;
      for(int m=0;m<sx.world.group[arg0-'A'].num_members;m++)
        if(sx.world.group[arg0-'A'].member[m]->hitpoints > 0) return 1;
      return 0;
    case C3_COND_KILLED:     // killed, <object>, <num>
      {
        int dead = 0;
        for(int m=0;m<sx.world.group[arg0-'A'].num_members;m++)
          if(sx.world.group[arg0-'A'].member[m]->hitpoints <= 0) dead++;
        return dead >= arg1;
      }
    case C3_COND_DESTROYED:
      if(arg0 == 'P') return P->hitpoints <= 0.0;
      for(int m=0;m<sx.world.group[arg0-'A'].num_members;m++)
        if(sx.world.group[arg0-'A'].member[m]->hitpoints > 0) return 0;
      return 1;
    case C3_COND_BELOW:      // below, <altitude>
      // triggers_printf(stderr, "checking below %g %u\n",
      //    m2ft(sx_heli_alt_above_ground(sx.world.player_move)), arg0);
      return m2ft(P->stat.alt_above_ground) < arg0;
    case C3_COND_ABOVE:      // above, <altitude>
      // triggers_printf(stderr, "checking above %g %d\n",
      // m2ft(sx_heli_alt_above_ground(sx.world.player_move)), arg0);
      return m2ft(P->stat.alt_above_ground) > arg0;
    case C3_COND_COUNTER:    // counter, <value>
      return mis->counter >= arg0;
    case C3_COND_TIME:       // time, <value>
      return mis->time >= arg0;
    case C3_COND_WAYPOINT:   // waypoint, <object>, <waypoint id>
      if(arg0 == 'P') return ((arg1&0xff)=='A') &&
        ((arg1>>8)==('1' + P->curr_wp-1)) &&
         (P->prev_wp != P->curr_wp);
      // TODO:
      // sx.world.group[arg0-'A'].members[m].curr_wp and curr_wpg ... but what happens if only one reaches the point?
      return 0;
    case C3_COND_DAMAGE:     // damage, <num>
      // TODO: return if hitpoints >= num
      if(P->hitpoints >= arg1) return 1;
      return 0;
    case C3_COND_ENCOUNTER:
      if(arg0 == 'P' && P->engaged != -1u) return sx.world.entity[P->engaged].id == arg1;
      return 0;
    case C3_COND_FACES:      // faces, <object/waypoint id>  // maybe looking at it?
      return 0;
    case C3_COND_WEAPON:
      return P->weapon == arg0;
    default:
      return 0;
  }
}

void
c3_triggers_print(
    c3_mission_t *mis,
    const c3_trigger_t *t)
{
  int i=0;
  do
  {
    triggers_printf(stderr, "%s %u %u is %d, %s\n",
        c3_condition_text[t->cond[i]], t->arg0[i], t->arg1[i],
        c3_triggers_check_cond(mis, t->cond[i], t->arg0[i], t->arg1[i]),
        c3_operator_text[t->oper[i]]);
    i++;
  }
  while(t->oper[i] != C3_OP_CNT);
}

void
c3_triggers_check_one(
    c3_mission_t *mis,
    c3_trigger_t *t)
{
  int cond = 1;
  int i = 0;
  do
  {
    int c = c3_triggers_check_cond(mis, t->cond[i], t->arg0[i], t->arg1[i]);
    if(t->oper[i] == C3_OP_OR)
    {
      if(cond) break; // already true
      cond = c; // better luck with second operand?
    }
    else if(t->oper[i] == C3_OP_AND)
    {
      cond &= c;
    }
    else if(t->oper[i] == C3_OP_ANDNOT)
    {
      cond &= !c;
    }
    i++;
  }
  while(t->oper[i] != C3_OP_CNT);
  if(cond)
  {
    t->enabled = 0; // trigger only fires once
    triggers_printf(stderr, "[trigger] %s\n", c3_action_text[t->action]);
    c3_triggers_print(mis, t);
    c3_triggers_action(mis, t->action, t->aarg0, t->aarg1);
    triggers_printf(stderr, "==========\n");
  }
}

void
c3_triggers_check(
    c3_mission_t *mis)
{
  int enabled = 0;
  for(int t=0;t<mis->num_triggers;t++)
    if(mis->trigger[t].enabled)
      c3_triggers_check_one(mis, mis->trigger+t), enabled++;
  // triggers_printf(stderr, "ran %d/%d triggers\n", enabled, mis->num_triggers);
}


void c3_triggers_reset(c3_mission_t *mis)
{
  for(int t=0;t<mis->num_triggers;t++)
    mis->trigger[t].enabled = 1;
}

