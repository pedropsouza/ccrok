#ifndef CROK_NET_HG__
#define CROK_NET_HG__

#include "crok/config.h"
#include "crok/state.h"
#include "ivec.h"
#include <stdio.h>
#include <stdlib.h>

enum crok_net_packet_kind {
  CROK_NET_INVALID = 0,

  // Client side requests
  CROK_NET_CONNECT,
  CROK_NET_READY,
  CROK_NET_QUIT,

  // Game actions
  CROK_NET_POS,
  CROK_NET_LAY_DISC,
  CROK_NET_AIM_DISC,
  CROK_NET_RECALL_DISC,
  CROK_NET_SHOOT_DISC,

  // Server responses
  CROK_NET_STATEDUMP,
  CROK_NET_ACK,
  CROK_NET_DESYNC,

  CROK_NET_END,
};

struct crok_net_packet_header {
  enum crok_net_packet_kind kind;
  uint64_t ticknum;
  uint32_t statesum;
  struct sockaddr_in src;
};

struct crok_net_packet {
  struct crok_net_packet_header header;
  union {
    struct ivec pos;
    struct ivec lay_disc;
    struct ivec aim_disc;
    struct ivec shoot_disc;
    char *serstr;
  };
};

struct crok_net_context {
  uint64_t ticknum;
  uint32_t state_chksum;

  size_t buf_last;
  struct crok_net_packet buf[CROK_NETPACKET_MAX];

  int sockfd;
  struct sockaddr_in saddr;
  struct sockaddr_in players[4];
};

typedef struct crok_net_packet (*crok_net_producer_fun)(
    struct crok_net_context *nc);

// query the producer fun till it's empty, accumulating
// its packets in the net context
//
// returns the number of accumulated packets
int crok_net_accum(struct crok_net_context *nc, crok_net_producer_fun f);
void crok_net_tickdata(struct crok_net_context *nc, uint64_t ticknum,
                       uint32_t state_chksum);

// clean the net context in preparation for the next frame
void crok_net_clean(struct crok_net_context *nc);
void crok_net_print_context(FILE *stream, struct crok_net_context *nc);

struct crok_net_packet crok_net_recv(struct crok_net_context *nc);
void crok_net_send(struct crok_net_context *nc, struct crok_net_packet *p);

#undef CINPCKTHEAD
#endif // CROK_NET_HG__
