#pragma once
#include "file.h"

#include <stdio.h>
#include <stdlib.h>

// groups:
// P player
// E enemy target to win
// G player goal to win

// WP player position as waypoint
// U wingman
// R reinforments (apache)
// X chinook?
// Y m1a1

// TODO: maybe .jim files contain not only the waypoint coords but also
// the objects in the wp groups?

typedef struct c3_pos_t
{
  // expect position 0-8191
  // and maybe heading 0-259?
  // object type (refers to list of .ai files?)

  // 16 bit:
  // object id       8
  // group id        6
  // camp (blue/red) 2
  uint16_t bitfield;

  // 16 0x0000         2 3
  uint16_t zero0;
  // 16 flag           4 5 tile x?
  int16_t tilex;
  // 16 ?              6 7 (contains some entropy) x?
  uint16_t x;
  // 16 flag           8 9 tile y?
  int16_t tiley;
  // 16 ?             10 11 (some info here) y?
  uint16_t y;
  // 16 0             12 13
  uint16_t zero1;
  // 16 0             14 15  heading pitch bank anyone?
  uint16_t zero2;
  // 16 coord?        16 17
  int16_t what0;
  // 16 coord?        18 19
  int16_t heading;
  // 4x16 0           20-28
  uint16_t flags[4];

  // could also have:
  // wp group
  // wp num
  // player goal
  // enemy goal
  // no fire
  // 360 aware
  // quick aim
  // flank left
  // slow fire
  // limited ammo
  // rockets only
  // quick movement

  // in map and map label? might be in .ord..
} 
c3_pos_t;

static inline int c3_pos_objectid(const c3_pos_t *p)
{
  return p->bitfield & 0xff;
}
static inline int c3_pos_groupid(const c3_pos_t *p)
{
  return (p->bitfield >> 8) & 0x3f;
}
static inline int c3_pos_campid(const c3_pos_t *p)
{
  return p->bitfield >> 14;
}


