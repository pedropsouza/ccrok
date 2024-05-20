#include "crok/net.h"
#include "crok.h"

pbuf_t *crok_net_preamble_put(netpacket *pckt, crok_net_preamble *pre) {
  assert(pckt->data_len == 0);
  pckt->data_len = pack(pckt->data, 8, CROK_NET_PREAMBLE_FMT,
                        &pre->ticknum, &pre->statesum, &pre->conn_uid);
  return pckt->data + pckt->data_len;
}
pbuf_t *crok_net_preamble_take(netpacket *pckt, crok_net_preamble *pre) {
  unpack(pckt->data, 8, CROK_NET_PREAMBLE_FMT,
         &pre->ticknum, &pre->statesum, &pre->conn_uid);
  return pckt->data + pack_unpack(
    NULL, SERDE_MODE_SIZEHINT, 8, CROK_NET_PREAMBLE_FMT, NULL, NULL);
}

void
broadcast(struct crok_data *cd) {
  for (int i = 0; i < 4; i++) {
    if (!(cd->crok.players[i].flags & CROK_U_VALID)) continue;
    cd->net.peer = cd->srv.socks[i];
    netmgr_send(&cd->net);
  }
}

