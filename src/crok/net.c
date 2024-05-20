#include "crok/net.h"
#include "crok.h"

void
broadcast(struct crok_data *cd) {
  for (int i = 0; i < 4; i++) {
    if (!(cd->crok.players[i].flags & CROK_U_VALID)) continue;
    cd->net.peer = cd->srv.socks[i];
    netmgr_send(&cd->net);
  }
}

void crok_netpacket_prepare_tx(crok_netpacket *pckt) {
  pckt->data_len = pack(pckt->data, 8, CROK_NET_PREAMBLE_FMT,
                        &pckt->ticknum, &pckt->statesum, &pckt->conn_uid);
  pckt->body_ptr = pckt->data + pckt->data_len;
}
void crok_netpacket_prepare_rx(crok_netpacket *pckt) {
  pckt->body_ptr = pckt->data
    + unpack(pckt->data, 8, CROK_NET_PREAMBLE_FMT,
             &pckt->ticknum, &pckt->statesum, &pckt->conn_uid);
}
