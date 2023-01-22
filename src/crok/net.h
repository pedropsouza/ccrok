#ifndef CROK_NET_HG__
#define CROK_NET_HG__

#include "crok/config.h"
#include "crok/globaltypes.h"
#include "crok/state.h"
#include "ivec.h"
#include "serde.h"
#include <string.h>
#include <errno.h>

struct crok_net_pcktrecord {
  struct sockaddr_in origin;
  struct crok_net_packet pckt;
};

struct crok_net_context {
  struct crok_net_packet tx;
  struct crok_net_packet rx;

  pbuf_t serbuf[sizeof(struct crok_net_packet)];

  size_t circular_head;
  struct crok_net_pcktrecord buf[CROK_NETPACKET_MAX];

  int sockfd;
  struct sockaddr_in saddr;
  struct sockaddr_in players[4];
};

typedef struct crok_net_pcktrecord (*crok_net_producer_fun)(void* data);

// query the producer fun till it's empty, accumulating
// its packets in the net context
//
// returns the number of accumulated packets
int crok_net_accum(struct crok_net_context *nc, crok_net_producer_fun f, void* f_data);

void crok_net_print_context(FILE *stream, struct crok_net_context *nc);

void
crok_net_tx_to_server(struct crok_net_context *nc);

void
crok_net_tx_to_addr(struct crok_net_context *nc, struct sockaddr_in *addr);

void
crok_net_tx_to_player(struct crok_net_context *nc, char player_id);

void
crok_net_tx_to_game(struct crok_net_context *nc);

struct sockaddr_in
crok_net_rx(struct crok_net_context *nc);

void
crok_net_print_packet(
  FILE *stream,  
  struct crok_net_packet *pckt);

void
crok_net_print_pcktrecord(
  FILE *stream,  
  struct crok_net_pcktrecord *r);

#undef CINPCKTHEAD
#endif // CROK_NET_HG__
