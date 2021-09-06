#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/crok.h"
#include "memutil.h"
#include "sched.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  struct crok_setup_data setdat = {.width = 800, .height = 450};

  if (argc > 1 &&
      (sscanf(argv[1], "%dx%d", &setdat.width, &setdat.height) != 2)) {
    goto invalid_arg;
  }
  printf("%dx%d\n", setdat.width, setdat.height);

  struct scheduler s =
      sched_init(crok_setup, crok_cleanup, crok_fixed, crok_render, 1e9 / 200,
                 1e9 / 100, &setdat, "main");

  sched_handover(&s);
  sched_debugprint(stdout, &s);

  return EXIT_SUCCESS;

invalid_arg:
  fprintf(
      stderr,
      "Bad arguments, usage:\n\t$ %s [<WIDTH>x<HEIGHT>]\ne.g.\t$ %s 1720x968\n",
      argv[0], argv[0]);
  return EXIT_FAILURE;
}
