#include "crok/input.h"

int crok_input_accum(struct crok_input_context *ic, struct crok_state *cs,
                     crok_input_producer_fun f, uint64_t ticknum) {
  struct crok_input_packet p;
  size_t old = ic->buf_last;
  size_t left = CROK_INPUTPACKET_MAX - ic->buf_last;
  for (; left > 0; left--) {
    p = f(cs, ticknum);
    if (p.header.kind == CROK_INPUT_END) {
      break;
    }
    if (p.header.kind != CROK_INPUT_INVALID) {
      int last_valid =
          (ic->buf[ic->buf_last].header.kind != CROK_INPUT_INVALID);
      size_t new_last = ic->buf_last + last_valid;
      ic->buf[new_last] = p;
      ic->buf_last = new_last;
    }
  }
  if (!left) {
    fprintf(stderr, "max input packets per frame exceeded!\n");
  }
  return ic->buf_last - old;
}

void crok_input_clean(struct crok_input_context *ic) {
  ic->buf[0] = (struct crok_input_packet){0};
  ic->buf_last = 0;
}

void crok_input_print_context(FILE *stream, struct crok_input_context *ic) {
  for (size_t i = 0; i < CROK_INPUTPACKET_MAX; i++) {
    if (i == ic->buf_last) {
      fprintf(stream, "[%d]-", ic->buf[i].header.kind);
    } else {
      fprintf(stream, "%d-", ic->buf[i].header.kind);
    }
  }
  fprintf(stream, "\n");
}
