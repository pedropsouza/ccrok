#ifndef CROK_INPUT_HG__
#define CROK_INPUT_HG__

struct crok_input_packet;
enum crok_input_packet_kind {
  CROK_INPUT_INVALID = 0,
  CROK_INPUT_MOUSE,
  CROK_INPUT_QUIT,
};

struct crok_input_mouse {
  enum crok_input_packet_kind kind;
  int x, y;
};
struct crok_input_packet crok_input_mouse_packet(int x, int y);

struct crok_input_quit {
  enum crok_input_packet_kind kind;
};
struct crok_input_packet crok_input_quit_packet();

struct crok_input_packet {
  union {
    enum crok_input_packet_kind kind;
    struct crok_input_mouse mouse;
    struct crok_input_quit quit;
  };
};

#endif // CROK_INPUT_HG__
