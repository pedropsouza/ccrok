#include "mahjongcell/hotreload.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int fixed_retval;
void fixed_adaptor(void *p) {
  struct { solr *d; long tps; } *d = p;
  fixed_retval = solr_fixed(d->d, d->tps);
}

void render_adaptor(void* p) {
  struct { solr *d; double a; } *d = p;
  solr_render(d->d, d->a);
}

void nullrender_adaptor(void* p) {}

int main(int argc, char *argv[]) {
  setup_data setdat = {
      .window_w = 850, .window_h = 800, .server = 0, .server_a = {0},
      .username = "anon", .username_len = sizeof(setdat.username), .frametime_ns = 1e9/30, .rdr_backend = -1 };
  // default address
  inet_pton(AF_INET, "127.0.0.1", &setdat.server_a.sin_addr);
  setdat.server_a.sin_family = AF_INET;
  setdat.server_a.sin_port = htons(SOLR_PORT);

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
      strncpy(setdat.username, optarg, SOLR_NAME_LEN);
      setdat.username_len = strlen(setdat.username);
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

#ifdef ENABLE_HOTRELOAD
  if(!reload_hot_functions()) {
    fprintf(stderr, "this error is unrecoverable! quitting.\n");
    return EXIT_FAILURE;
  }
#endif

  solr cd = solr_setup(&setdat);
  if (cd.st.stage != STAGE_PLAYING) { return EXIT_FAILURE; }
  struct periodic_callback fixed_call = {0};
  fixed_call.func = fixed_adaptor;
  fixed_call.arg = (&(struct { solr *d; long tps; }){.d = &cd, .tps = setdat.frametime_ns});
  fixed_call.max_per_tick = 10;
  fixed_call.strat = PERIODIC_CALLBACK_DEFAULT;
  fixed_call.period_ns = setdat.frametime_ns;

  struct periodic_callback render_call = {0};
  render_call.func = setdat.server ? nullrender_adaptor : render_adaptor;
  render_call.arg = (&(struct { solr *d; double a; }){.d = &cd, .a = 0.0});
  render_call.strat = PERIODIC_CALLBACK_NOACC;
  render_call.period_ns = setdat.frametime_ns;
  
  do {
    struct timespec wait_f = do_periodic_callback(&fixed_call);
    do_periodic_callback(&render_call);
#ifdef ENABLE_HOTRELOAD
    if (cd.should_reload) {
      solr_pre_reload(&cd);
      if (!reload_hot_functions()) {
        fprintf(stderr, "this error is unrecoverable! quitting.\n");
        return EXIT_FAILURE;
      }
      solr_post_reload(&cd);
      cd.should_reload = 0;
    }
#endif
    nanosleep(&wait_f, NULL);
  } while (fixed_retval);

  solr_cleanup(&cd);

  return EXIT_SUCCESS;

invalid_arg:
  fprintf(stderr,
          "Bad arguments, usage:\n\t$ %s [-s] [-a <address>] "
          "[-d <WIDTH>x<HEIGHT>] [-u <username>]\ne.g.\t$ %s -d "
          "1720x968 -u bob\n",
          argv[0], argv[0]);
  return EXIT_FAILURE;
}
