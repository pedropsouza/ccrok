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
      .width = 800, .height = 450, .servermode = 0, .saddr = {0}, .name = ""};
  // default address
  inet_pton(AF_INET, "127.0.0.1", &setdat.saddr.sin_addr);
  setdat.saddr.sin_family = AF_INET;
  setdat.saddr.sin_port = htons(CROK_PORT);

  int r;
  while ((r = getopt(argc, argv, "sa:d:u:")) != -1) {
    switch (r) {
    case 's':
      setdat.servermode = 1;
      break;
    case 'a':
      if (inet_pton(AF_INET, optarg, &setdat.saddr.sin_addr) == 0) {
        fprintf(stderr, "Invalid address \"%s\"\n", optarg);
        goto invalid_arg;
      }
      break;
    case 'd':
      if (sscanf(optarg, "%dx%d", &setdat.width, &setdat.height) != 2) {
        goto invalid_arg;
      }
      break;
    case 'u':
      strncpy(setdat.name, optarg, CROK_PLAYER_NAME_MAXLEN);
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

  struct scheduler s =
      sched_init(crok_setup, crok_cleanup, crok_fixed, renf, 1e9 / CROK_TPS,
                 1e9 / CROK_FPS, &setdat, "main");

  sched_handover(&s);
  sched_debugprint(stdout, &s);

  return EXIT_SUCCESS;

invalid_arg:
  fprintf(stderr,
          "Bad arguments, usage:\n\t$ %s [-s] [-a <address>] "
          "[-d <WIDTH>x<HEIGHT>] [-u <username>]\ne.g.\t$ %s -d "
          "1720x968 -u bob\n",
          argv[0], argv[0]);
  return EXIT_FAILURE;
}
