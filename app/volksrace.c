#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "volksrace/volksrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void headless_noop(struct scheduler *s, double a) {}

int main(int argc, char *argv[]) {
  setup_data setdat = {
      .window_w = 850, .window_h = 800, .server = 0, .server_a = {0},
      .username = "anon", .frametime_ns = 1e9/30, .rdr_backend = -1 };
  // default address
  inet_pton(AF_INET, "127.0.0.1", &setdat.server_a.sin_addr);
  setdat.server_a.sin_family = AF_INET;
  setdat.server_a.sin_port = htons(PORT);

  int r;
  while ((r = getopt(argc, argv, "sa:d:u:p:b:")) != -1) {
    switch (r) {
    case 's':
      setdat.server = 1;
      break;
    case 'a':
      if (inet_pton(AF_INET, optarg, &setdat.server_a.sin_addr) == 0) {
        fprintf(stderr, "Invalid address \"%s\"\n", optarg);
        goto invalid_arg;
      }
      break;
    case 'd':
      if (sscanf(optarg, "%lux%lu", &setdat.window_w, &setdat.window_h) != 2) {
        goto invalid_arg;
      }
      break;
    case 'u':
      strncpy(setdat.username, optarg, NAME_LEN);
      break;
    case 'p':
      if (sscanf(optarg, "%hu", &setdat.server_a.sin_port) != 1) goto invalid_arg;
      setdat.server_a.sin_port = htons(setdat.server_a.sin_port);
      break;
    case 'b':
      if (sscanf(optarg, "%d", &setdat.rdr_backend) != 1) goto invalid_arg;
      setdat.rdr_backend = setdat.rdr_backend < 0 ? -1 : setdat.rdr_backend;
      break;
    case '?':
      goto invalid_arg;
      break;
    default:
      abort();
      break;
    }
  }

  render_fun renf = (setdat.server) ? headless_noop : volks_render;

  struct scheduler s =
      sched_init(volks_setup, volks_cleanup, volks_fixed, renf, setdat.frametime_ns,
                 setdat.frametime_ns, &setdat, "main");

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
