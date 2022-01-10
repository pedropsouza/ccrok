#ifndef CROK_HG__
#define CROK_HG__

#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/data.h"
#include "crok/gloop.h"
#include "crok/net.h"
#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include "serde.h"
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#define CROK_PORT 33850

struct crok_net_packet crok_local_input(struct crok_net_context *ic);
struct crok_net_packet crok_network_input(struct crok_net_context *ic);

struct sched_trans crok_setup(struct scheduler *s);
struct sched_trans crok_cleanup(struct scheduler *s);
struct sched_trans crok_fixed(struct scheduler *s, long step_ns);

void crok_render(struct scheduler *s, double a);

enum crok_gamestage crok_process_client(struct crok_data *d);
enum crok_gamestage crok_process_server(struct crok_data *d);

/* Utility functions, return <0 on error */
size_t crok_spawn_player(struct crok_state *cs, struct crok_player p);
size_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner);

#endif // CROK_HG__
