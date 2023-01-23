#include "crok/net.h"

int
crok_net_accum(struct crok_net_context *nc, crok_net_producer_fun f, void *f_data) {
  struct crok_net_pcktrecord r;
  size_t n;
  for (n = 0; n < CROK_NETPACKET_MAX;) {
    r = f(f_data);
    if (r.pckt.kind == CROK_NET_END) {
      break;
    }
    if (r.pckt.kind != CROK_NET_INVALID) {
      size_t new_head = (nc->circular_head + 1) % CROK_NETPACKET_MAX;
      nc->buf[new_head] = r;
      nc->circular_head = new_head;
      n++;
    } else {
      fprintf(stderr, "received an invalid packet\n");
      crok_net_print_packet(stderr, &r.pckt);
    }
  }
  if (n == CROK_NETPACKET_MAX) {
    fprintf(stderr, "too many packets accumulated per tick! dropped some.");
  }
  return n;
}

void
crok_net_print_context(FILE *stream, struct crok_net_context *nc) {
  for (size_t i = 0; i < CROK_NETPACKET_MAX; i++) {
    if (i == nc->circular_head) {
      fprintf(stream, "[%lu] - ", i);
      crok_net_print_pcktrecord(stream, nc->buf + i);
    } else {
      fprintf(stream, "%lu - ", i);
      crok_net_print_pcktrecord(stream, nc->buf + i);
    }
  }
  fprintf(stream, "\n");
}

static size_t
tx_pack(struct crok_net_context *nc) {
  return netpacket_pack(nc->serbuf, &(nc->tx));
}

static void
rx_unpack(struct crok_net_context *nc) {
  netpacket_unpack(nc->serbuf, &(nc->rx));
}

static void
procret(ssize_t v,const char *fn) {
  if (v < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      fprintf(stderr, "%s would block, ignoring\n",fn);
    } else {
      perror(fn);
    }
  }
}

static ssize_t
tx_to_addr(struct crok_net_context *nc, size_t sz, struct sockaddr_in *addr) {
  return sendto(
    nc->sockfd, nc->serbuf, sz, 0,
    (struct sockaddr *)addr, sizeof(struct sockaddr_in));
}

void
crok_net_tx_to_server(struct crok_net_context *nc) {
  size_t p_size = tx_pack(nc);
  ssize_t retval = tx_to_addr(nc, p_size, &nc->saddr);
  procret(retval, __func__);
}

void
crok_net_tx_to_addr(struct crok_net_context *nc, struct sockaddr_in *addr) {
  size_t p_size = tx_pack(nc);
  ssize_t retval = tx_to_addr(nc, p_size, addr);
  procret(retval, __func__);
}

void
crok_net_tx_to_player(struct crok_net_context *nc, char player_id) {
  size_t p_size = tx_pack(nc);
  ssize_t retval = tx_to_addr(nc, p_size, nc->players + player_id);
  procret(retval, __func__);
}

void
crok_net_tx_to_game(struct crok_net_context *nc) {
  size_t p_size = tx_pack(nc);
  for (int i = 0; i < 4; i++) {
    if (nc->players[i].sin_addr.s_addr != 0) {
      ssize_t retval = tx_to_addr(nc, p_size, nc->players + i);
      procret(retval, __func__);
    }
  }
}

// leaves the buffers dirty on the early returns!
struct sockaddr_in
crok_net_rx(struct crok_net_context *nc) {
  socklen_t discarded = sizeof(struct sockaddr_in);
  struct sockaddr_in origin = {0};
  ssize_t rbytes = recvfrom(
    nc->sockfd, nc->serbuf, sizeof(nc->serbuf), 0,
    (struct sockaddr *)&origin, &discarded);
  if (rbytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      nc->rx.kind = CROK_NET_END; goto end;
    } else {
      perror(__func__);
      nc->rx.kind = CROK_NET_INVALID; goto end;
    }
  }
  rx_unpack(nc);
  end:
  return origin;
}

void
crok_net_print_packet(FILE *stream, struct crok_net_packet *pckt) {
  fprintf(stream, "tick %lu sum %u id %hhu kind %hhu length %hu\n",
    pckt->ticknum, pckt->statesum,
    pckt->conn_uid, (char)pckt->kind, pckt->data_len);
}

void
crok_net_print_pcktrecord(FILE *stream, struct crok_net_pcktrecord *r) {
  char s[INET_ADDRSTRLEN];
  fprintf(stream, "Packet from \"%s:%hu\": ",
      inet_ntop(r->origin.sin_family, &r->origin.sin_addr, s, sizeof(s)),
      r->origin.sin_port);
  crok_net_print_packet(stream, &r->pckt);
}
