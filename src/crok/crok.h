#ifndef CROK_HG__
#define CROK_HG__

#include <unistd.h>
#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/data.h"
#include "crok/globaltypes.h"
#include "crok/gloop.h"
#include "crok/net.h"
#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include "serde.h"

#define CROK_PORT 33850

struct crok_tick_data { uint64_t tnum; uint32_t tsum; };
struct crok_netinput_data { struct crok_net_context *nc; struct crok_tick_data *td; };
//struct crok_net_packet crok_local_input(void *data);
//struct crok_net_packet crok_network_input(void *data);

struct sched_trans crok_setup(struct scheduler *s);
struct sched_trans crok_cleanup(struct scheduler *s);
struct sched_trans crok_fixed(struct scheduler *s, long step_ns);

uint32_t crok_simulate(struct crok_state *cs);
void crok_render(struct scheduler *s, double a);

enum crok_gamestage crok_process(struct crok_data *d);

/* Utility functions, return <0 on error */
ssize_t crok_spawn_player(struct crok_state *cs, struct crok_player p);
ssize_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner);

#endif // CROK_HG__
