#include "crok/hotreload.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int fixed_retval;
void fixed_adaptor(void *p) {
  struct { struct crok_data *d; long tps; } *d = p;
  fixed_retval = crok_fixed(d->d, d->tps);
}

void render_adaptor(void* p) {
  struct { struct crok_data *d; double a; } *d = p;
  crok_render(d->d, d->a);
}

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

#ifdef ENABLE_HOTRELOAD
  if(!reload_hot_functions()) {
    fprintf(stderr, "this error is unrecoverable! quitting.\n");
    return EXIT_FAILURE;
  }
#endif

  struct crok_data cd = crok_setup(&setdat);
  struct periodic_callback fixed_call = {0};
  fixed_call.func = fixed_adaptor;
  fixed_call.arg = (&(struct { struct crok_data *d; long tps; }){.d = &cd, .tps = 1e9 / CROK_TPS});
  fixed_call.max_per_tick = 10;
  fixed_call.strat = PERIODIC_CALLBACK_DEFAULT;
  fixed_call.period_ns = 1e9/CROK_TPS;

  struct periodic_callback render_call = {0};
  render_call.func =render_adaptor;
  render_call.arg = (&(struct { struct crok_data *d; double a; }){.d = &cd, .a = 0.0});
  render_call.strat = PERIODIC_CALLBACK_NOACC;
  render_call.period_ns = 1e9/CROK_FPS;
  
  do {
    struct timespec wait_f = do_periodic_callback(&fixed_call);
    do_periodic_callback(&render_call);
#ifdef ENABLE_HOTRELOAD
    if (cd.should_reload) {
      //crok_pre_reload(&cd);
      if (!reload_hot_functions()) {
        fprintf(stderr, "this error is unrecoverable! quitting.\n");
        return EXIT_FAILURE;
      }
      //crok_post_reload(&cd);
      cd.should_reload = 0;
    }
#endif
    nanosleep(&wait_f, NULL);
  } while (fixed_retval);

  crok_cleanup(&cd);

  return EXIT_SUCCESS;

invalid_arg:
  fprintf(stderr,
          "Bad arguments, usage:\n\t$ %s [-s] [-a <address>] "
          "[-d <WIDTH>x<HEIGHT>] [-u <username>]\ne.g.\t$ %s -d "
          "1720x968 -u bob\n",
          argv[0], argv[0]);
  return EXIT_FAILURE;
}
