#pragma once

// every host has their list of entities they're responsible for.
// send packets peer/peer or via server, don't know yet.
// divide up responsibilities between host ids.
// TODO: need way to identify hosts, one guy has to work as the "server"
// TODO: how to identify entities uniquely?

// TODO: split entity list into parts that are owned by the individual clients?

typedef struct sx_net_t
{
  // TODO: stuff and then store it as sx.net
  uint32_t num_clients;
  uint32_t client[32];
}
sx_net_t;

// TODO: init everything we need for state
// TODO: establish connection to some given host, maybe using TCP side channels.
// TODO: handshake and exchange host/port for udp packets to listen to.
void sx_net_init_server();
void sx_net_init_client();

void sx_net_recv();
// SDLNet_UDP_Recv // non blocking and returns 0 if nothing there, -1 on error
// TODO: read packet, with:
// timestamp, hostid
// list of rigid bodies of entities
// list of movement controllers of (possibly different?) entities
// discard if our entity has newer timestamp or belongs to our hostid

void sx_net_send();
// SDLNet_UDP_Send
// pack udp packet with timestamp, hostid, and:
// for all entities in our responsibility:
// rigid body + movement controller + entity state (52 + 72 + 20 = 144 bytes)
// stop every 512 bytes and use new packet

// TODO: what's in a packet?
typedef struct sx_udp_entity_t
{
  uint32_t eid;    // entity id to find it in global list
  // if eid does not exist, create it. if it should be deleted, there will be a
  // secret flag hidden below, TODO
  // for this to work, we'll need a pool of dynamic entities that are owned by
  // every host separately.

  // primary quantities of rigid body:
  float c[3];      // center of mass in world space [m]
  quat_t q;        // orientation object to world space
  float pv[3];     // momentum in world space
  float pw[3];     // angular momentum in world space

  // movement controller stuff:
  sx_move_controller_t move;

  // entity state stuff:
  uint32_t prev_wp, curr_wp, weapon, birth_time;
  float hitpoints;
}
sx_udp_entity_t;

typedef struct sx_udp_packet_t
{
  uint32_t time;
  sx_udp_entity_t ent[3]; // seems we can't fit more than 3 in one packet
}
sx_udp_packet_t;

