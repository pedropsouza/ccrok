#include "sched.h"

struct scheduler sched_init(setup_fun s, fixed_fun f, render_fun r,
                            cleanup_fun c, double timestep, void *setup_data) {
  struct scheduler sched = {0};
  sched.setup = s;
  sched.fixed = f;
  sched.render = r;
  sched.cleanup = c;
  sched.setup_data = setup_data;

  sched.t = 0.0;
  sched.dt = timestep;
  sched.current_time = SDL_GetTicks() / 1000.0;
  sched.accumulator = 0.0;

  sched.trans_hist[0] =
      (struct sched_trans){.s = SCHED_SETUP, .why = "sched_init call"};

  enum sched_state new_st = sched_exec_till_change(&sched);
  assert(new_st == SCHED_CONTINUE);
  return sched;
}

int sched_enqueue(struct scheduler *s, struct sched_trans t) {
  if (sched_get_newest_state(s) != t.s) {
    size_t head = s->trans_queue_head_idx;
    size_t tail = s->trans_queue_tail_idx;
    size_t target = head;
    const size_t max_s = SCHED_TRANS_QUEUE_SIZE;
    if (s->trans_queue[head].s != SCHED_NONE) {
      target = head + 1 % max_s;
      if (target == tail) {
        // queue overflow! The next tick should abort.
        // clobbler the start of the queue
        s->trans_queue[tail] = (struct sched_trans){
            .s = SCHED_ERR, .why = "trans_queue overflow!"};
        return 0;
      }
    }
    s->trans_queue[target] = t;
    s->trans_queue_head_idx = target;
    return 1;
  }
  return 0;
}

int sched_exec(struct scheduler *s) {
  double new_time, frame_time;
  uint32_t ticks_this_call = 0;

  switch (sched_get_cur_state(s)) {
  case SCHED_NONE:
    sched_enqueue(s,
                  (struct sched_trans){.s = SCHED_ERR,
                                       .why = "tried to tick with no state!"});
    break;
  case SCHED_SETUP:
    sched_enqueue(s, s->setup(s));
    break;
  case SCHED_CLEANUP:
    sched_enqueue(s, s->cleanup(s));
    break;

  case SCHED_CONTINUE:
    // TODO FIXME: implement state interpolation
    new_time = SDL_GetTicks() / 1000.0;
    frame_time = new_time - s->current_time;
    // if (frame_time > 0.25) {
    //  frame_time = 0.25;
    //}
    s->current_time = new_time;

    s->accumulator += frame_time;

    while (s->accumulator >= s->dt) {
      sched_enqueue(s, s->fixed(s, s->dt));
      s->t += s->dt;
      s->accumulator -= s->dt;
      ticks_this_call += 1;
      if (ticks_this_call > SCHED_TRANS_QUEUE_SIZE) {
        printf(
            "warning, %d/%d ticks done, possible transition queue overflow!\n",
            ticks_this_call, SCHED_TRANS_QUEUE_SIZE);
      }
    }

    s->render(s, 0.0);

    break;
  case SCHED_PAUSE:
    // noop, wait for external state change
    break;

  case SCHED_HALT:
    if (sched_isclean(s)) {
      return 0; // no further ticks required
    } else {
      sched_enqueue(s, (struct sched_trans){
                           .s = SCHED_ERR,
                           .why = "tried to halt without cleaning up first!"});
    }
    break;
  case SCHED_ERR:
    fprintf(stderr, "SCHED_ERR reached!");
    sched_debugprint(stderr, s);
    abort();
    break;
  }
  // consume enqueued transitions from oldest to newest
  size_t head = s->trans_queue_head_idx;
  size_t tail = s->trans_queue_tail_idx;
  struct sched_trans tail_trans = s->trans_queue[tail];
  if (tail_trans.s != SCHED_NONE) {
    // consumed slots must be reset to SCHED_NONE
    s->trans_queue[tail] = (struct sched_trans){0};
    // make it the current state
    size_t hist = (s->trans_hist_idx + 1) % SCHED_TRANS_HISTORY_SIZE;
    s->trans_hist[hist] = tail_trans;
    s->trans_hist_idx = hist;

    if (head == tail) {
      head = (tail + 1) % SCHED_TRANS_QUEUE_SIZE;
    }
    tail = (tail + 1) % SCHED_TRANS_QUEUE_SIZE;

    s->trans_queue_head_idx = head;
    s->trans_queue_tail_idx = tail;
  }

  return 1; // more ticks are necessary
}

enum sched_state sched_exec_till_change(struct scheduler *s) {
  enum sched_state old, new;
  old = new = sched_get_cur_state(s);
  do {
    if (sched_exec(s) == 0) {
      return SCHED_HALT;
    }
    new = sched_get_cur_state(s);
  } while (new == old);
  return new;
}

void sched_exit(struct scheduler *s) {
  sched_enqueue(
      s, (struct sched_trans){.s = SCHED_CLEANUP, .why = "sched_exit call"});
  if (sched_exec_till_change(s) != SCHED_HALT) {
    sched_enqueue(
        s, (struct sched_trans){
               .s = SCHED_ERR,
               .why = "sched_exit couldn't transition from cleanup to halt!"});
    sched_exec(s);
  }
}

enum sched_state sched_get_cur_state(struct scheduler *s) {
  return s->trans_hist[s->trans_hist_idx].s;
}

enum sched_state sched_get_newest_state(struct scheduler *s) {
  enum sched_state queued = s->trans_queue[s->trans_queue_head_idx].s;
  if (queued == SCHED_NONE) {
    return sched_get_cur_state(s);
  }
  return queued;
}

int sched_isclean(struct scheduler *s) { return (s->persistant_data == NULL); }

// TODO: implement only printing up to tail
void sched_print_circular_buffer(FILE *stream, struct sched_trans *buf,
                                 size_t head, const size_t cap) {
  for (int i = 0; i < cap; i++) {
    size_t idx = (cap + head - i) % cap;
    struct sched_trans t = buf[idx];
    fprintf(stream, "\t#%d - %s reason \"%s\"\n", i, sched_enum_name(t.s),
            t.why);
  }
}

void sched_debugprint(FILE *stream, struct scheduler *s) {
  fprintf(stream, "scheduler %x with timestep %f\n", *(unsigned int *)s, s->dt);
  fprintf(stream, "transition queue:\n");
  sched_print_circular_buffer(stream, s->trans_queue, s->trans_queue_head_idx,
                              SCHED_TRANS_QUEUE_SIZE);

  fprintf(stream, "transition history:\n");
  sched_print_circular_buffer(stream, s->trans_hist, s->trans_hist_idx,
                              SCHED_TRANS_HISTORY_SIZE);
}

const char *sched_enum_name(enum sched_state st) {
  // TODO maybe transform into a macro?
  switch (st) {
  case SCHED_NONE:
    return "NONE";
  case SCHED_SETUP:
    return "SETUP";
  case SCHED_CLEANUP:
    return "CLEANUP";
  case SCHED_CONTINUE:
    return "CONTINUE";
  case SCHED_PAUSE:
    return "PAUSE";
  case SCHED_HALT:
    return "HALT";
  case SCHED_ERR:
    return "ERR";
  }
}
