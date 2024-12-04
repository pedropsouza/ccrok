#include "net.h"
#include <stdlib.h>

size_t
netpacket_pack(pbuf_t *dst, netpacket *pckt) {
  char fmt[32] = NETPACKET_FMTSTR_PARTIAL;
  size_t plen = strlen(fmt);
  snprintf(fmt + plen, sizeof(fmt) - plen, "%zus", pckt->data_len);
  return pack(dst, alignof(netpacket),
    fmt, &pckt->kind, &pckt->crc, &pckt->tick, &pckt->data
  );
}
size_t
netpacket_unpack(pbuf_t *src, netpacket *pckt) {
  return unpack(src, alignof(netpacket),
          NETPACKET_FMTSTR_PARTIAL "s?", &pckt->kind, &pckt->crc, &pckt->tick, &pckt->data, &pckt->data_len);
}

static size_t
netmgr_retcheck(ssize_t v, const char *fn_name) {
  if (v < 0) {  
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      fprintf(stderr, "%s would block, ignoring\n", fn_name);
      return -1;
    } else {
      perror(fn_name);
      return 0;
    }
  }
  return v;
}

static size_t
tx_pack(netmgr *n) {
  return netpacket_pack(n->serde_buf, n->tx);
}

static size_t
rx_unpack(netmgr *n) {
  return netpacket_unpack(n->serde_buf, n->rx);
}

static inline ssize_t
innersend(netmgr *n) {
  return send(n->sock_fd, n->serde_buf, tx_pack(n), 0);
}

static inline ssize_t
innerrecv(netmgr *n) {
  ssize_t rtv = recv(n->sock_fd, n->serde_buf, sizeof(n->serde_buf), 0);
  if (rtv > 0) {
    rx_unpack(n);
  }
  return rtv;
}

// -1 if would block, 0 for error, > 0 n of bytes sent
ssize_t
netmgr_send(netmgr *n) {
  ssize_t rtv = innersend(n);
  return netmgr_retcheck(rtv, __func__);
}

// -1 if would block, 0 for error, > 0 n of bytes received
ssize_t
netmgr_recv(netmgr *n) {
  ssize_t rtv = innerrecv(n);
  return netmgr_retcheck(rtv, __func__);
}

// returns n of bytes sent
size_t
netmgr_sprecv(netmgr *n) {
  ssize_t rtv = innerrecv(n);
  if (rtv == -1) {
    n->rx->kind = n->spkinds.onblock;
    // FIXME: error checking
    return 0;
  } else if (rtv == 0) {
    n->rx->kind = n->spkinds.ondisconnect;
    perror(__func__);
    return 0;
  }
  return rtv;
}

char *netpacket_owned_debug_str(netpacket *pckt, const char *(*kind_to_name)(int)) {
  const char *fmtstr = "packet kinded %d datalen %zu := ";
  const char *pckt_name = "";
  if (kind_to_name != NULL) {
    pckt_name = kind_to_name(pckt->kind);
    fmtstr = "packet %s datalen %zu := ";
  }
  // null char, description, name and data space, + smudge factor.
  size_t out_len = 1 + strlen(fmtstr) + strlen(pckt_name) + pckt->data_len*2 + 16 + pckt->data_len/8+1;
  char *s = malloc(out_len);
  size_t head;
  if (kind_to_name != NULL) {
    head = snprintf(s, out_len, fmtstr, pckt_name, pckt->data_len);
  } else {
    head = snprintf(s, out_len, fmtstr, pckt->kind, pckt->data_len);
  }
  for (size_t i = 0; i < pckt->data_len; i++) {
    head += sprintf(s+head, "%s%02x", (i%8 == 0)? " " : "", (unsigned char)pckt->data[i]);
  }
  return s;
}
