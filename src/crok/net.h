#ifndef CROK_NET_HG__
#define CROK_NET_HG__

#include "crok/config.h"
#include "crok/globaltypes.h"
#include "crok/state.h"
#include "ivec.h"
#include "serde.h"
#include "../net.h"
#include <string.h>
#include <errno.h>
#include <assert.h>

struct crok_server_data {
  struct sockaddr_in players[4];
};

#define CROK_NET_PREAMBLE_FMT "QIC"
#define CROK_NET_PREAMBLE_FIELDS \
  uint64_t ticknum; \
  uint32_t statesum; \
  uint8_t conn_uid;

typedef struct {
  CROK_NET_PREAMBLE_FIELDS
} crok_net_preamble;

typedef struct {
  NETPACKET_FIELDS
  pbuf_t *body_ptr;
  CROK_NET_PREAMBLE_FIELDS
} crok_netpacket;

// returns the remaining usable space on the packet buffer
pbuf_t *crok_net_preamble_put(netpacket *pckt, crok_net_preamble *pre);
// returns the rest of the data that is yet to be read
pbuf_t *crok_net_preamble_take(netpacket *pckt, crok_net_preamble *pre);

struct crok_data;
void broadcast(struct crok_data *cd);

void crok_netpacket_prepare_tx(crok_netpacket *packet);
void crok_netpacket_prepare_rx(crok_netpacket *packet);

#endif // CROK_NET_HG__
