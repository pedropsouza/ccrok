#ifndef CROK_INPUT_HG__
#define CROK_INPUT_HG__

#include "crok/config.h"
#include "ivec.h"
#include <stdio.h>
#include <stdlib.h>

#define CPK enum crok_input_packet_kind kind
#define CIP struct crok_input_packet

CIP;
enum crok_input_packet_kind {
  CROK_INPUT_INVALID = 0,
  CROK_INPUT_POS,
  CROK_INPUT_LAY_DISC,
  CROK_INPUT_AIM_DISC,
  CROK_INPUT_RECALL_DISC,
  CROK_INPUT_SHOOT_DISC,
  CROK_INPUT_READY,
  CROK_INPUT_QUIT,
  CROK_INPUT_END,
};

struct crok_input_pos {
  CPK;
  struct ivec pos;
};

struct crok_input_lay_disc {
  CPK;
  struct ivec pos;
};

struct crok_input_aim_disc {
  CPK;
  char disc_id;
  struct ivec delta;
};

struct crok_input_shoot_disc {
  CPK;
  char disc_id;
  struct ivec impulse;
};

CIP {
  union {
    CPK;
    struct crok_input_pos pos;
    struct crok_input_lay_disc lay_d;
    struct crok_input_aim_disc aim_d;
    struct crok_input_shoot_disc shoot_d;
  };
};

// for future connection tracking or something
struct crok_input_context {
  size_t buf_last;
  CIP buf[CROK_INPUTPACKET_MAX];
};

typedef CIP (*crok_input_producer_fun)(struct crok_input_context *ic,
                                       void *extra);

// query the producer fun till it's empty, accumulating
// its packets in the input context
//
// returns the number of accumulated packets
int crok_input_accum(struct crok_input_context *ic, crok_input_producer_fun f,
                     void *extra);

// clean the input context in preparation for the next frame
void crok_input_clean(struct crok_input_context *ic);
void crok_input_print_context(FILE *stream, struct crok_input_context *ic);

#undef CIP
#undef CPK
#endif // CROK_INPUT_HG__
