#ifndef CROK_CONFIG_HG__
#define CROK_CONFIG_HG__

// these are in game units, with each unit being a UINT32_MAX-th of the board
// diameter, including the ditch
#define CROK_BOARD_DIAM (UINT32_MAX)
#define CROK_DITCH_DIAM (CROK_BOARD_DIAM / 24)
#define CROK_HOLE_DIAM (CROK_BOARD_DIAM / 28)

// WARN
// The total can't be an expression as it is copied verbatim to a format string
// in serde.c
#define CROK_DISCS_PER_SIDE 12
#define CROK_DISCS_TOTAL 24

#define CROK_DISC_DIAM (CROK_BOARD_DIAM / 32)
#define CROK_POST_DIAM (CROK_BOARD_DIAM / 64)
#define CROK_PLAYER_NAME_MAXLEN 40
#define PHYSBODY_NAME_MAXLEN 10
#define CROK_NETPACKET_MAX 32
#define CROK_ROLLBACK_MAX 48

#define CROK_TPS 50
#define CROK_FPS 50
#define CROK_PACKET_MAX_DATA_SIZE 8192

// For expanding macros inside string literals
#define MACROSTR2(X) #X
#define MACROSTR(X) MACROSTR2(X)

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
