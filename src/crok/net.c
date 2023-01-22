#include "crok/net.h"

int crok_net_accum(struct crok_net_context *nc, crok_net_producer_fun f, void *f_data) {
  struct crok_net_packet p;
  size_t old_head = nc->circular_head;
  size_t n;
  for (n = 0; n < CROK_NETPACKET_MAX; n++) {
    p = f(f_data);
    if (p.kind == CROK_NET_END) {
      break;
    }
    if (p.kind != CROK_NET_INVALID) {
      int last_valid = (nc->buf[nc->circular_head].kind != CROK_NET_INVALID);
      size_t new_head = (nc->circular_head + last_valid) % CROK_NETPACKET_MAX;
      nc->buf[new_head] = p;
      nc->circular_head = new_head;
    } else {
      fprintf(stderr, "received an invalid packet\n");
      crok_net_print_packet(stderr, &p);
    }
  }
  if (n == CROK_NETPACKET_MAX) {
    fprintf(stderr, "too many packets accumulated per tick! dropped some.");
  }
  return n;
}

void crok_net_print_context(FILE *stream, struct crok_net_context *nc) {
  for (size_t i = 0; i < CROK_NETPACKET_MAX; i++) {
    if (i == nc->circular_head) {
      fprintf(stream, "[%d]-", nc->buf[i].kind);
    } else {
      fprintf(stream, "%d-", nc->buf[i].kind);
    }
  }
  fprintf(stream, "\n");
}

static size_t tx_pack(struct crok_net_context *nc) {
  return netpacket_pack(nc->serbuf, &(nc->tx));
}

static void rx_unpack(struct crok_net_context *nc) {
  netpacket_unpack(nc->serbuf, &(nc->rx));
}
void crok_net_tx_to_server(struct crok_net_context *nc) {
  size_t p_size = tx_pack(nc);
  ssize_t retval = sendto(
    nc->sockfd, nc->serbuf, p_size, 0,
    (struct sockaddr *)&(nc->saddr), sizeof(struct sockaddr_in));
  if (retval < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      fprintf(stderr, "%s would block, ignoring\n", __func__);
    } else {
      perror(__func__);
    }
  }
}

static ssize_t tx_to_player(struct crok_net_context *nc, size_t sz, char pid) {
  return sendto(
    nc->sockfd, nc->serbuf, sz, 0,
    (struct sockaddr *)nc->players + pid, sizeof(struct sockaddr_in));
}

static void procret(ssize_t v,const char *fn) {
  if (v < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      fprintf(stderr, "%s would block, ignoring\n",fn);
    } else {
      perror(fn);
    }
  }
}

void
crok_net_tx_to_player(struct crok_net_context *nc, char player_id) {
  size_t p_size = tx_pack(nc);
  ssize_t retval = tx_to_player(nc, p_size, player_id);
  procret(retval, __func__);
}

void
crok_net_tx_to_game(struct crok_net_context *nc) {
  size_t p_size = tx_pack(nc);
  for (int i = 0; i < 4; i++) {
    ssize_t retval = tx_to_player(nc, p_size, i);
    procret(retval, __func__);
  }
}

// leaves the buffers dirty on the early returns!
void
crok_net_rx(struct crok_net_context *nc) {
  ssize_t rbytes = recv(nc->sockfd, nc->serbuf, CROK_PACKET_DATA_SIZE, 0);
  if (rbytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      nc->rx.kind = CROK_NET_END; return;
    } else {
      perror("net error");
      nc->rx.kind = CROK_NET_INVALID; return;
    }
  }
  rx_unpack(nc);
}

void
crok_net_print_packet(FILE *stream, struct crok_net_packet *pckt) {
  fprintf(stream, "tick %lu sum %u id %hhu kind %hhu\n",
    pckt->ticknum, pckt->statesum,
    pckt->conn_uid, (char)pckt->kind);
}
