#include "crok.h"

struct crok_input_packet crok_local_input(struct crok_input_context *ic) {
  SDL_Event input;

  if (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      return crok_input_quit_packet();
      break;
    case SDL_MOUSEMOTION:
      return crok_input_mouse_packet(input.motion.x, input.motion.y);
      break;
    default:
      return (struct crok_input_packet){0};
      break;
    }
  }
  return crok_input_end_packet();
}

struct sched_trans crok_setup(struct scheduler *s) {
  s->persistant_data =
      safe_malloc(sizeof(struct crok_data), "setup persistant_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  if (!setdat->servermode) {
    int width = setdat->width, height = setdat->height;
    SDL_Init(SDL_INIT_VIDEO);
    d->window = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, width, height, 0);

    d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(d->renderer);

    // get the possibly modified dimensions
    SDL_GetRendererOutputSize(d->renderer, &width, &height);
    d->width = width;
    d->height = height;

    SDL_Surface *surface;
    surface = IMG_Load("resources/ul.png");
    d->tex = SDL_CreateTextureFromSurface(d->renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(d->tex, NULL, NULL, &d->dest.w, &d->dest.h);
    d->dest.w /= 2;
    d->dest.h /= 2;
    d->dest.x = (width - d->dest.w) / 2;
    d->dest.y = (height - d->dest.h) / 2;
  }

  d->crok = crok_state_init(d);
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  if (!setdat->servermode) {
    SDL_DestroyTexture(d->tex);
    SDL_DestroyRenderer(d->renderer);
    SDL_DestroyWindow(d->window);
    SDL_Quit();
  }

  d = NULL;
  free(s->persistant_data);
  s->persistant_data = NULL;

  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans crok_fixed(struct scheduler *s, long step_ns) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  static long num_ticks = 0;
  num_ticks += 1;
  if (num_ticks * step_ns >= 10 * (long)1e9) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "10 seconds elapsed"};
  }

  crok_input_accum(&d->input, crok_local_input);
  for (size_t i = 0; i <= d->input.buf_last; i++) {
    struct crok_input_packet p = d->input.buf[i];
    switch (p.kind) {
    case CROK_INPUT_INVALID:
      break;
    case CROK_INPUT_MOUSE:
      d->dest.x = p.mouse.x - d->dest.w / 2;
      d->dest.y = p.mouse.y - d->dest.h / 2;
      break;
    case CROK_INPUT_QUIT:
      return (struct sched_trans){.s = SCHED_CLEANUP,
                                  .why = "user requested quit"};
      break;
    default:
      break;
    }
  }

  crok_input_clean(&d->input);

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;

  SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(d->renderer);

  SDL_RenderCopy(d->renderer, d->crok.board.tex, NULL, &d->crok.board.dest);
  SDL_RenderCopy(d->renderer, d->tex, NULL, &d->dest);

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(d->renderer);
}
