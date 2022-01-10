#include "crok/net.h"
#include <string.h>

int crok_net_accum(struct crok_net_context *nc, crok_net_producer_fun f) {
  struct crok_net_packet p;
  size_t old = nc->buf_last;
  size_t left = CROK_NETPACKET_MAX - nc->buf_last;
  for (; left > 0; left--) {
    p = f(nc);
    if (p.header.kind == CROK_NET_END) {
      break;
    }
    if (p.header.kind != CROK_NET_INVALID) {
      int last_valid = (nc->buf[nc->buf_last].header.kind != CROK_NET_INVALID);
      size_t new_last = nc->buf_last + last_valid;
      nc->buf[new_last] = p;
      nc->buf_last = new_last;
    }
  }
  if (!left) {
    fprintf(stderr, "max net packets per frame exceeded!\n");
  }
  return nc->buf_last - old;
}

void crok_net_tickdata(struct crok_net_context *nc, uint64_t tncknum,
                       uint32_t state_chksum) {
  nc->ticknum = tncknum;
  nc->state_chksum = state_chksum;
}

void crok_net_clean(struct crok_net_context *nc) {
  nc->buf[0] = (struct crok_net_packet){0};
  nc->buf_last = 0;
}

void crok_net_print_context(FILE *stream, struct crok_net_context *nc) {
  for (size_t i = 0; i < CROK_NETPACKET_MAX; i++) {
    if (i == nc->buf_last) {
      fprintf(stream, "[%d]-", nc->buf[i].header.kind);
    } else {
      fprintf(stream, "%d-", nc->buf[i].header.kind);
    }
  }
  fprintf(stream, "\n");
}

struct crok_net_packet crok_net_recv(struct crok_net_context *nc) {}

void crok_net_send(struct crok_net_context *nc, struct crok_net_packet *p);
