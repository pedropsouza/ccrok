#include "mahjcell_net.h"

ssize_t mahjcell_send_with(
  solr *s, int kind,
  pbuf_t *state_ser, size_t state_ser_len,
  uint32_t state_crc)
{
  netmgr *n = &s->net;
  netpacket *tx = n->tx;
  
  tx->crc = state_crc;
  tx->tick = s->st.tick;
  tx->kind = kind;

  switch (kind) {
    case PCKT_STDUMP: {
      memcpy(tx->data, state_ser, state_ser_len);
      tx->data_len = state_ser_len;
      return netmgr_send(n);
    }
    case PCKT_CANCEL: {
      tx->data_len = pack(tx->data, 1, "C", &s->client.uid);
    } /* fallthrough */
    case PCKT_RESTARTREQ: /* fallthrough */
    case PCKT_FFWREQ: /* fallthrough */
    case PCKT_LEAVE: /* fallthrough */
    case PCKT_POS: /* fallthrough */
    case PCKT_ACK: /* fallthrough */
    case PCKT_MOVE: /* fallthrough */
    case PCKT_FFWANIM: /* fallthrough */
    case PCKT_JOIN: /* fallthrough */
    case PCKT_DISCONNECT: /* fallthrough */
    case PCKT_INVALID:
      return netmgr_send(n);
    default: {
      fprintf(stderr, "%s: unknown packet kind %d!\n", __func__, kind);
      return -1;
    }
  }
}

ssize_t mahjcell_send(solr *s, int kind) {
  netmgr *n = &s->net;
  netpacket *tx = n->tx;
  pbuf_t state_ser[NETPACKET_MAX_PAYLOAD];
  size_t state_ser_len = pack(state_ser, 8, "G", &s->st);
  uint32_t crc = xcrc32(state_ser, state_ser_len, 0xffffffff);
  return mahjcell_send_with(s, kind, state_ser, state_ser_len, crc);
}

ssize_t mahjcell_broadcast_with(
  solr *s, int kind,
  pbuf_t *state_ser, size_t state_ser_len,
  uint32_t state_crc)
{
  netmgr *n = &s->net;
  netpacket *tx = n->tx;

  tx->tick = s->st.tick;
  tx->crc = state_crc;
  tx->kind = kind;

  ssize_t acc = 0;
  for (int i = 0; i < SOLR_REAL_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.sock_fd = s->srv.sock_fds[i];
    if (s->net.tx->kind == PCKT_POS) {
      acc += mahjcell_send_with(s, PCKT_POS, state_ser, state_ser_len, state_crc);
      continue;
    }
    char *pcktstr = netpacket_owned_debug_str(
        s->net.tx,
        packet_kind_to_name
      );
    if (s->net.tx->kind != PCKT_STDUMP) {
      printf("broadcasting %s to %d, fd %d)\n", pcktstr, i, s->net.sock_fd);
    } else {
      printf("broadcasting statedump to %d, fd %d\n", i, s->net.sock_fd);
    }
    acc += mahjcell_send_with(s, kind, state_ser, state_ser_len, state_crc);
    free(pcktstr);
  }
  return acc;
}

ssize_t mahjcell_broadcast(solr *s, int kind) {
  netmgr *n = &s->net;
  netpacket *tx = n->tx;
  pbuf_t state_ser[NETPACKET_MAX_PAYLOAD];
  size_t state_ser_len = pack(state_ser, 8, "G", &s->st);
  uint32_t crc = xcrc32(state_ser, state_ser_len, 0xffffffff);
  return mahjcell_broadcast_with(s, kind, state_ser, state_ser_len, crc);
}

