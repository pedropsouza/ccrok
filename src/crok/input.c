#include "crok/input.h"

struct crok_input_packet crok_input_mouse_packet(int x, int y) {
  return (struct crok_input_packet){
      .mouse = {.kind = CROK_INPUT_MOUSE, .x = x, .y = y}};
}

struct crok_input_packet crok_input_quit_packet() {
  return (struct crok_input_packet){.quit = {.kind = CROK_INPUT_QUIT}};
}
