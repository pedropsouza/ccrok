#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/crok.h"
#include "memutil.h"
#include "sched.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void headless_noop(struct scheduler *s, double a) {}

int main(int argc, char *argv[]) {
  struct crok_setup_data setdat = {
      .width = 800, .height = 450, .servermode = 0};

  int r;
  while ((r = getopt(argc, argv, "sd:")) != -1) {
    switch (r) {
    case 's':
      setdat.servermode = 1;
      break;
    case 'd':
      if (sscanf(optarg, "%dx%d", &setdat.width, &setdat.height) != 2) {
        goto invalid_arg;
      }
      break;
    case '?':
      goto invalid_arg;
      break;
    default:
      abort();
      break;
    }
  }

  render_fun renf = (setdat.servermode) ? headless_noop : crok_render;

  struct scheduler s = sched_init(crok_setup, crok_cleanup, crok_fixed, renf,
                                  1e9 / 200, 1e9 / 100, &setdat, "main");

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
