#ifndef CROK_CONFIG_HG__
#define CROK_CONFIG_HG__
#include <stdint.h>

// these are in game units, with each unit being a UINT32_MAX-th of the board
// diameter, including the ditch
#define CROK_BOARD_DIAM (UINT32_MAX)
#define CROK_DITCH_DIAM (CROK_BOARD_DIAM / 24)
#define CROK_HOLE_DIAM (CROK_BOARD_DIAM / 28)

#define CROK_DISCS_PER_SIDE 12
#define CROK_DISCS_TOTAL (CROK_DISCS_PER_SIDE * 2)
#define CROK_DISC_DIAM (CROK_BOARD_DIAM / 32)
#define CROK_POST_DIAM (CROK_BOARD_DIAM / 64)
#define CROK_NAMELEN_MAX 32
#define CROK_INPUTPACKET_MAX 128

#define CROK_TPS 100
#define CROK_FPS 100

// Debugging aid
#ifdef NDEBUG
#define DEBUG 0
#else
#define DEBUG 1
#endif
#define PRINTLVAR(NAME)                                                        \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printf(#NAME " = %ld\n", NAME);                                          \
  } while (0)
#define PRINTFVAR(NAME)                                                        \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printf(#NAME " = %f\n", NAME);                                           \
  } while (0)

#endif // CROK_CONFIG_HG__

// TODO: Wrap SDL_CreateTexture() so it reports errors
