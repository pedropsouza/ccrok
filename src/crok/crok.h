#ifndef CROK_HG__
#define CROK_HG__

#include "crok/data.h"
#include "crok/disc.h"
#include "crok/input.h"
#include "crok/player.h"
#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

struct crok_input_packet crok_local_input(struct crok_input_context *ic);
struct sched_trans crok_setup(struct scheduler *s);
struct sched_trans crok_cleanup(struct scheduler *s);
struct sched_trans crok_fixed(struct scheduler *s, long step_ns);
void crok_render(struct scheduler *s, double a);

#endif // CROK_HG__
