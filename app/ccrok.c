#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok.h"
#include "memutil.h"
#include "sched.h"
#include <stdio.h>
#include <stdlib.h>

struct app_data {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *tex;
  SDL_Rect dest;
};

struct sched_trans setup(struct scheduler *s) {
  s->persistant_data =
      safe_malloc(sizeof(struct app_data), "setup persistant_data alloc");
  struct app_data *d = (struct app_data *)s->persistant_data;

  SDL_Init(SDL_INIT_VIDEO);
  d->window = SDL_CreateWindow("SDL2Test", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

  d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_SOFTWARE);
  SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(d->renderer);

  // creates a surface to load an image into the main memory
  SDL_Surface *surface;

  // please provide a path for your image
  surface = IMG_Load("resources/ul.png");

  // loads image to our graphics hardware memory.
  d->tex = SDL_CreateTextureFromSurface(d->renderer, surface);

  // clears main-memory
  SDL_FreeSurface(surface);

  // connects our texture with dest to control position
  SDL_QueryTexture(d->tex, NULL, NULL, &d->dest.w, &d->dest.h);

  // adjust height and width of our image box.
  d->dest.w /= 2;
  d->dest.h /= 2;

  // sets initial x-position of object
  d->dest.x = (640 - d->dest.w) / 2;

  // sets initial y-position of object
  d->dest.y = (480 - d->dest.h) / 2;
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans cleanup(struct scheduler *s) {
  struct app_data *d = (struct app_data *)s->persistant_data;

  // destroy texture and window
  SDL_DestroyTexture(d->tex);
  SDL_DestroyWindow(d->window);

  d = NULL;
  free(s->persistant_data);
  s->persistant_data = NULL;

  SDL_Quit();
  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans fixed(struct scheduler *s, double step) {
  struct app_data *d = (struct app_data *)s->persistant_data;
  static int num_ticks = 0;
  num_ticks += 1;
  if (num_ticks * step >= 10.0) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "10 seconds elapsed"};
  }

  SDL_Event input;

  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      return (struct sched_trans){.s = SCHED_CLEANUP,
                                  .why = "user requested quit"};
      break;
    case SDL_MOUSEMOTION:
      d->dest.x = input.motion.x - d->dest.w / 2;
      d->dest.y = input.motion.y - d->dest.h / 2;
      break;
    }
  }

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void render(struct scheduler *s, double a) {
  struct app_data *d = (struct app_data *)s->persistant_data;

  SDL_RenderClear(d->renderer);
  // renders the texture at dest coords
  SDL_RenderCopy(d->renderer, d->tex, NULL, &d->dest);
  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(d->renderer);
}

int main(int argc, char *argv[]) {
  struct scheduler s =
      sched_init(setup, fixed, render, cleanup, 1.0 / 100.0, NULL);

  while (sched_exec(&s))
    ;
  sched_debugprint(stdout, &s);

  return EXIT_SUCCESS;
}
