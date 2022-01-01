#ifndef CROK_INPUT_HG__
#define CROK_INPUT_HG__

#include "crok/config.h"
#include "crok/state.h"
#include "ivec.h"
#include <stdio.h>
#include <stdlib.h>

enum crok_input_packet_kind {
  CROK_INPUT_INVALID = 0,

  CROK_INPUT_REGISTER,
  CROK_INPUT_STATEDUMP,

  CROK_INPUT_POS,
  CROK_INPUT_LAY_DISC,
  CROK_INPUT_AIM_DISC,
  CROK_INPUT_RECALL_DISC,
  CROK_INPUT_SHOOT_DISC,
  CROK_INPUT_READY,
  CROK_INPUT_QUIT,
  CROK_INPUT_END,
};

struct crok_input_packet_header {
  enum crok_input_packet_kind kind;
  uint64_t ticknum;
  uint32_t statesum;
  uint32_t pcktsum;
};

struct crok_input_packet {
  struct crok_input_packet_header header;
  union {
    struct ivec pos;
    struct ivec lay_disc;
    struct ivec aim_disc;
    struct ivec shoot_disc;
    char serstr[CROK_STATE_SERLEN];
  };
};

struct crok_input_context {
  size_t buf_last;
  struct crok_input_packet buf[CROK_INPUTPACKET_MAX];
};

typedef struct crok_input_packet (*crok_input_producer_fun)(
    struct crok_state *cs, uint64_t ticknum);

// query the producer fun till it's empty, accumulating
// its packets in the input context
//
// returns the number of accumulated packets
int crok_input_accum(struct crok_input_context *ic, struct crok_state *cs,
                     crok_input_producer_fun f, uint64_t ticknum);

// clean the input context in preparation for the next frame
void crok_input_clean(struct crok_input_context *ic);
void crok_input_print_context(FILE *stream, struct crok_input_context *ic);

#undef CINPCKTHEAD
#endif // CROK_INPUT_HG__
