#ifndef NET_HG__
#define NET_HG__

#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "serde.h"

#ifndef NETPACKET_MAX_PAYLOAD
#define NETPACKET_MAX_PAYLOAD 8192+256
#endif
#define NETPACKET_FIELDS \
  uint32_t kind; \
  uint32_t crc; \
  uint32_t tick; \
  uint32_t data_len; \
  pbuf_t data[NETPACKET_MAX_PAYLOAD];

typedef struct {
  NETPACKET_FIELDS
} netpacket;

typedef struct {
  uint32_t onblock;
  uint32_t onerror; // FIXME: this one is not really used, even though it should
  uint32_t ondisconnect;
} netspecialkinds;

#define NETPACKET_FMTSTR_PARTIAL "III"
size_t netpacket_pack  (pbuf_t *dst, netpacket *pckt);
size_t netpacket_unpack(pbuf_t *src, netpacket *pckt);
char *netpacket_owned_debug_str(netpacket *pckt, const char *(*kind_to_name)(int));

typedef struct {
  int sock_fd;
  alignas(8) pbuf_t serde_buf[sizeof(netpacket)];
  netpacket *tx;
  netpacket *rx;
  netspecialkinds spkinds;
} netmgr;

ssize_t netmgr_send(netmgr *n);
ssize_t netmgr_recv(netmgr *n);
// special receive, uses the set netspecialkinds
size_t netmgr_sprecv(netmgr *n);

#endif // NET_HG__
