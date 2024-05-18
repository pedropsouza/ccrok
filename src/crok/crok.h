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
#include "sched.h"
#include "serde.h"

#define CROK_PORT 33850

struct crok_tick_data { uint64_t tnum; uint32_t tsum; };
struct crok_netinput_data { struct crok_net_context *nc; struct crok_tick_data *td; };
//struct crok_net_packet crok_local_input(void *data);
//struct crok_net_packet crok_network_input(void *data);

#ifndef ENABLE_HOTRELOAD
struct crok_data crok_setup(struct crok_setup_data *);
void crok_cleanup(struct crok_data *);
int crok_fixed(struct crok_data *, long step_ns);
void crok_render(struct crok_data *, double a);
#endif // ENABLE_HOTRELOAD

void crok_simulate(struct crok_state *cs);
enum crok_gamestage crok_process(struct crok_data *d);

/* Utility functions, return <0 on error */
ssize_t crok_spawn_player(struct crok_state *cs, struct crok_player p);
ssize_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner);

// Hot reloading bits -- all tsoding's musializer code
// https://github.com/tsoding/musializer/blob/master/src/hotreload.h
#ifdef ENABLE_HOTRELOAD

#define HOT_FUNCS \
HOT_FUNC(crok_setup, struct crok_data, struct crok_setup_data *) \
HOT_FUNC(crok_cleanup, void, struct crok_data *) \
HOT_FUNC(crok_fixed, int, struct crok_data *, long) \
HOT_FUNC(crok_render, void, struct crok_data *, double)

#endif // ENABLE_HOTRELOAD

#endif // CROK_HG__
