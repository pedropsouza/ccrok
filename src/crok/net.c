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

void
crok_net_send_packet(
      struct crok_net_context *nc,
      struct crok_net_packet *pckt)
{
  pbuf_t serbuf[sizeof(struct crok_net_packet)];
  size_t p_size = netpacket_pack(serbuf, pckt);
  ssize_t retval = send(nc->sockfd, serbuf, p_size, 0);
  if (retval < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      fprintf(stderr, "crok_net_send_packet would block, ignoring\n");
    } else {
      perror("crok_net_send_packet");
    }
  }
}

void
crok_net_print_packet(FILE *stream, struct crok_net_packet *pckt) {
  fprintf(stream, "tick %lu sum %u id %hhu kind %hhu\n",
    pckt->ticknum, pckt->statesum,
    pckt->conn_uid, (char)pckt->kind);
}
