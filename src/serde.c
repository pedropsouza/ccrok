#include "serde.h"

size_t crok_gstage_serfun(pbuf_t *head, char mode, void* io) {
  struct crok_gstage *iot = io;
  size_t (*f) (pbuf_t *, size_t, char *, ...) = (mode == SERDE_MODE_PACK)? pack : unpack;
  size_t align = alignof(struct crok_gstage);
  switch (iot->kind) {
    case CROK_GS_LOBBY:
    case CROK_GS_SIDESEL:
      return (*f)(head, align, "C", &iot->data.players_ready);
    case CROK_GS_ROUND:
      return (*f)(head, align, "C", &iot->data.active_player);
    default:
      return 0;
  }
}

#define NETPACKET_FMTSTR_PARTIAL "QICC"
size_t netpacket_pack(pbuf_t *dst, struct crok_net_packet *pckt) {
  char fmt[32] = NETPACKET_FMTSTR_PARTIAL;
  size_t plen = strlen(fmt);
  snprintf(fmt + plen, sizeof(fmt) - plen, "%us", pckt->data_len);
  return pack(dst, alignof(struct crok_net_packet), fmt,
          &pckt->ticknum, &pckt->statesum,
          &pckt->conn_uid, &pckt->kind, &pckt->data);
}
size_t netpacket_unpack(pbuf_t *src, struct crok_net_packet *pckt) {
  return unpack(src, alignof(struct crok_net_packet),
          NETPACKET_FMTSTR_PARTIAL "s?", &pckt->ticknum, &pckt->statesum,
          &pckt->conn_uid, &pckt->kind, &pckt->data, &pckt->data_len);
}

#define IVEC_FMTSTR "QQ"
size_t ivec_pack(pbuf_t *dst, struct ivec *v) {
  return pack(dst, alignof(struct ivec), IVEC_FMTSTR, &v->x, &v->y);
}
size_t ivec_unpack(pbuf_t *src, struct ivec *v) {
  return unpack(src, alignof(struct ivec), IVEC_FMTSTR, &v->x, &v->y);
}

#define FVEC_FMTSTR "II"
size_t fvec_pack(pbuf_t *dst, struct fvec *v) {
  return pack(dst, alignof(struct fvec), FVEC_FMTSTR, &v->x, &v->y);
}
size_t fvec_unpack(pbuf_t *src, struct fvec *v) {
  return unpack(src, alignof(struct fvec), FVEC_FMTSTR, &v->x, &v->y);
}

#define PHYSBODY_FMTSTR "vvvvI" MACROSTR(PHYSBODY_NAME_MAXLEN) "s"
size_t physbody_pack(pbuf_t *dst, struct physbody *pb) {
  return pack(dst, alignof(struct physbody), PHYSBODY_FMTSTR,
          &pb->pos, &pb->vel, &pb->acc, &pb->facc, &pb->radius, &pb->name);
}
size_t physbody_unpack(pbuf_t *src, struct physbody *pb) {
  return unpack(src, alignof(struct physbody), PHYSBODY_FMTSTR,
          &pb->pos, &pb->vel, &pb->acc, &pb->facc, &pb->radius, &pb->name);
}

#define CROK_PLAYER_FMTSTR "vIH" MACROSTR(CROK_PLAYER_NAME_MAXLEN) "s"
size_t crok_player_pack(pbuf_t *dst, struct crok_player *p) {
  return pack(dst, alignof(struct crok_player), CROK_PLAYER_FMTSTR,
          &p->pos, &p->score, &p->flags, &p->name);
}
size_t crok_player_unpack(pbuf_t *src, struct crok_player *p) {
  return unpack(src, alignof(struct crok_player), CROK_PLAYER_FMTSTR,
          &p->pos, &p->score, &p->flags, &p->name);
}

#define CROK_DISC_FMTSTR "pI"
size_t crok_disc_pack(pbuf_t *dst, struct crok_disc *d) {
  return pack(dst, alignof(struct crok_disc), CROK_DISC_FMTSTR, &d->pb, &d->flags);
}
size_t crok_disc_unpack(pbuf_t *src, struct crok_disc *d) {
  return unpack(src, alignof(struct crok_disc), CROK_DISC_FMTSTR, &d->pb, &d->flags);
}

#define CROK_STATE_FMTSTR "4P" MACROSTR(CROK_DISCS_TOTAL) "DIIC[C]"
size_t crok_state_pack(pbuf_t *dst, struct crok_state *st) {
  return pack(dst, alignof(struct crok_state), CROK_STATE_FMTSTR,
              &st->players, &st->discs, &st->round,
              &st->overall_score, &st->total_players,
              &crok_gstage_serfun, &st->stage);
}

size_t crok_state_unpack(pbuf_t *src, struct crok_state *st) {
  size_t l = unpack(src, alignof(struct crok_state),
                CROK_STATE_FMTSTR,
                &st->players, &st->discs, &st->round,
                &st->overall_score, &st->total_players,
                &crok_gstage_serfun, &st->stage);
  return l;
}

static size_t pad_align(pbuf_t **head, int alignment) {
  size_t padl = 0;
  while((uintptr_t)*head % alignment != 0) {
    #ifdef DEBUG
    *(*head) = 0xad;
    #endif
    (*head)++;
    padl++;
  }
  return padl;
}

// calculates the naive size, without padding
size_t calcsize(char *fmt) {
  size_t a = 0;
  ssize_t valcount = 1;
  int counted = 0;
  for(; *fmt != '\0'; fmt++) {
    switch (*fmt) {
      case 'C': a += valcount * 1; break;
      case 'H': a += valcount * 2; break;
      case 'I': a += valcount * 4; break;
      case 'Q': a += valcount * 8; break;
      case 's': a += 2 + valcount; break;
      case 'v': a += valcount * calcsize(IVEC_FMTSTR); break;
      case 'p': a += valcount * calcsize(PHYSBODY_FMTSTR); break;
      case 'P': a += valcount * calcsize(CROK_PLAYER_FMTSTR); break;
      case 'D': a += valcount * calcsize(CROK_DISC_FMTSTR); break;
      // BROKEN, can't know for sure the size
      case 'S': a += valcount * calcsize(CROK_STATE_FMTSTR); break;
      default:
        if (isdigit(*fmt)) {
          if(!counted) { counted = 1; valcount = 0; };
          valcount = 10 * valcount + (*fmt - '0');
          continue;
        }
        break;
    }
    valcount = 1;
    counted = 0;
  }
  return a;
}

// C - byte
// H - u16
// I - u32
// Q - u64
// s - string
// v - ivec
// p - physbody
// P - player
// D - disc
// S - state
// [<kind>] - tagged struct
size_t pack(pbuf_t *dst, size_t align, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack(dst, SERDE_MODE_PACK, align, fmt, argl);
  va_end(argl);
  return len;
}


// C - byte
// H - u16
// I - u32
// Q - u64
// s - string
// v - ivec
// p - physbody
// P - player
// D - disc
// S - state
// ? interrogate length of last string
// [<kind>] - tagged struct
size_t unpack(pbuf_t *src, size_t align, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack(src, SERDE_MODE_UNPACK, align, fmt, argl);
  va_end(argl);
  return len;
}

size_t pack_unpack(pbuf_t *head, char mode, size_t align, char *fmt, va_list va) {
  union ser_t v;

  size_t slen;
  size_t len;
  ssize_t valcount = 0;

  size_t size = 0;

  size += pad_align(&head, align);
  assert((uintptr_t)head % align == 0);
  for (; *fmt != '\0'; fmt++) {
    switch (*fmt) {
    case 'C': // byte
      v.u8 = (uint8_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint8_t *)head) = *v.u8;
        } else {
          *v.u8 = *((uint8_t *)head);
        }
        head += sizeof(uint8_t);
        size += sizeof(uint8_t);
        v.u8++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'H': // u16
      size += pad_align(&head, alignof(uint16_t));
      v.u16 = (uint16_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint16_t *)head) = htons(*v.u16);
        } else {
          *v.u16 = ntohs(*((uint16_t *)head));
        }
        head += sizeof(uint16_t);
        size += sizeof(uint16_t);
        v.u16++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'I': // u32
      size += pad_align(&head, alignof(uint32_t));
      v.u32 = (uint32_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint32_t *)head) = htonl(*v.u32);
        } else {
          *v.u32 = ntohl(*((uint32_t *)head));
        }
        head += sizeof(uint32_t);
        size += sizeof(uint32_t);
        v.u32++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'Q': // u64
      size += pad_align(&head, alignof(uint64_t));
      v.u64 = (uint64_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint32_t *)head) = htonl((uint32_t)(0xffffffff & (*v.u64 >> 32)));
          head += sizeof(uint32_t);
          *((uint32_t *)head) = htonl((uint32_t)(0xffffffff & *v.u64));
          head += sizeof(uint32_t);
        } else {
          *v.u64 = (uint64_t)htonl(*((uint32_t *)head)) << 32;
          head += sizeof(uint32_t);
          *v.u64 |= (uint64_t)ntohl(*((uint32_t *)head));
          head += sizeof(uint32_t);
        }
        size += sizeof(uint64_t);
        v.u64++;
        valcount--;
      } while (valcount > 0);
      break;
    case '?': // interrogate length of last string
      if (mode == SERDE_MODE_UNPACK) {
        v.len = va_arg(va, uint32_t *);
        *v.len = slen;
      } else {
        fprintf(stderr, "'?' in a packing format string, shenanigans will ensue\n");
      }
      break;
    case 's': // string
      // WARN: won't accept a count on the format, already used for length
      size += pad_align(&head, alignof(uint32_t));
      v.str = va_arg(va, char *);
      if (mode == SERDE_MODE_PACK) {
        slen = valcount;
        *((uint32_t *)head) = htons((uint32_t)valcount);
        head += sizeof(uint32_t);
        memcpy(head, v.str, slen);
        head += slen;
        size += sizeof(uint32_t) + valcount;
      } else {
        slen = ntohs(*(uint32_t *)head);
        head += sizeof(uint32_t);
        memcpy(v.str, head, slen);
        head += slen;
        size += sizeof(uint32_t) + slen;
      }
      break;
    case 'v': // ivec
      v.v = (struct ivec *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = ivec_pack(head, v.v);
        } else {
          len = ivec_unpack(head, v.v);
        }
        head += len;
        size += len;
        v.v++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'p': // physbody
      v.pb = (struct physbody *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = physbody_pack(head, v.pb);
        } else {
          len = physbody_unpack(head, v.pb);
        }
        head += len;
        size += len;
        v.pb++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'P': // player
      v.p = (struct crok_player *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_player_pack(head, v.p);
        } else {
          len = crok_player_unpack(head, v.p);
        }
        head += len;
        size += len;
        v.p++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'D': // disc
      v.d = (struct crok_disc *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_disc_pack(head, v.d);
        } else {
          len = crok_disc_unpack(head, v.d);
        }
        head += len;
        size += len;
        v.d++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'S': // state
      v.st = (struct crok_state *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_state_pack(head, v.st);
        } else {
          len = crok_state_unpack(head, v.st);
        }
        head += len;
        size += len;
        v.st++;
        valcount--;
      } while (valcount > 0);
      break;
    case '[': { // begin tagged struct (doesn't respect multipliers!)
      // get the formatter function
      tagged_struct_serfun serfun =
          (tagged_struct_serfun)va_arg(va, tagged_struct_serfun);
      fmt++; // ignore [ marker
      assert(*fmt != '\0');
      char buf[] = { *fmt, '\0' };
      fmt++; // and the ] too

      void *io = (void*)va_arg(va, void*);
      union ser_t kind = (union ser_t)io;
      if (mode == SERDE_MODE_PACK) {
        len = pack(head, alignof(union ser_t), buf, kind);
      } else {
        len = unpack(head, alignof(union ser_t), buf, kind);
      }
      head += len;
      size += len;
      if (serfun != NULL) {
        len = (*serfun)(head, mode, io);
        head += len;
        size += len;
      }
      // ^ This routine is in dire need of an alignment optimization
    } break;
    default:
      if (isdigit(*fmt)) {
        valcount = 10 * valcount + (*fmt - '0');
        continue;
      }
      break;
    }
    valcount = 0;
  }
  return size;
}
