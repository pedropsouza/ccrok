
#ifndef GLOBALTYPES_HG__
#define GLOBALTYPES_HG__
#include "crok/config.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef unsigned char pbuf_t;

enum crok_net_packet_kind {
  CROK_NET_INVALID = 0,
  CROK_NET_IGNORE = 1,

  // Client side requests
  CROK_NET_CONNECT,
  CROK_NET_READY,
  CROK_NET_QUIT,

  // Game actions
  CROK_NET_CLIENT_POS,
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

struct crok_net_packet {
  uint64_t ticknum;
  pbuf_t data[CROK_PACKET_DATA_SIZE];
  uint32_t statesum;
  enum crok_net_packet_kind kind;
  uint8_t conn_uid;
};

#endif // GLOBALTYPES_HG__
