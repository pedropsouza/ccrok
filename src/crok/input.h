#ifndef CROK_INPUT_HG__
#define CROK_INPUT_HG__

#include "crok/config.h"
#include <stdio.h>
#include <stdlib.h>

struct crok_input_packet;
enum crok_input_packet_kind {
  CROK_INPUT_INVALID = 0,
  CROK_INPUT_MOUSE,
  CROK_INPUT_QUIT,
  CROK_INPUT_END,
};

struct crok_input_mouse {
  enum crok_input_packet_kind kind;
  int x, y;
};
struct crok_input_packet crok_input_mouse_packet(int x, int y);

struct crok_input_quit {
  enum crok_input_packet_kind kind;
};
struct crok_input_packet crok_input_quit_packet();

struct crok_input_end {
  enum crok_input_packet_kind kind;
};
struct crok_input_packet crok_input_end_packet();

struct crok_input_packet {
  union {
    enum crok_input_packet_kind kind;
    struct crok_input_mouse mouse;
    struct crok_input_quit quit;
    struct crok_input_end end;
  };
};

// for future connection tracking or something
struct crok_input_context {
  size_t buf_last;
  struct crok_input_packet buf[CROK_INPUTPACKET_MAX];
};

typedef struct crok_input_packet (*input_producer_fun)(
    struct crok_input_context *ic);

// query the producer fun till it's empty, accumulating
// its packets in the input context
//
// returns the number of accumulated packets
int crok_input_accum(struct crok_input_context *ic, input_producer_fun f);

// clean the input context in preparation for the next frame
void crok_input_clean(struct crok_input_context *ic);
void crok_input_print_context(FILE *stream, struct crok_input_context *ic);

#endif // CROK_INPUT_HG__
