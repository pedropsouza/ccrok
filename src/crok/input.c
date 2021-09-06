#include "crok/input.h"

struct crok_input_packet crok_input_mouse_packet(int x, int y) {
  return (struct crok_input_packet){
      .mouse = {.kind = CROK_INPUT_MOUSE, .x = x, .y = y}};
}

struct crok_input_packet crok_input_quit_packet() {
  return (struct crok_input_packet){.quit = {.kind = CROK_INPUT_QUIT}};
}

struct crok_input_packet crok_input_end_packet() {
  return (struct crok_input_packet){.end = {.kind = CROK_INPUT_END}};
}

int crok_input_accum(struct crok_input_context *ic, input_producer_fun f) {
  struct crok_input_packet p;
  size_t old = ic->buf_last;
  size_t left = CROK_INPUTPACKET_MAX - ic->buf_last;
  for (; left > 0; left--) {
    p = f(ic);
    if (p.kind == CROK_INPUT_END) {
      break;
    }
    if (p.kind != CROK_INPUT_INVALID) {
      size_t new_last =
          ic->buf_last + (ic->buf[ic->buf_last].kind != CROK_INPUT_INVALID);
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
      fprintf(stream, "[%d]-", ic->buf[i].kind);
    } else {
      fprintf(stream, "%d-", ic->buf[i].kind);
    }
  }
  fprintf(stream, "\n");
}
