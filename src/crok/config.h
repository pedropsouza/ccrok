#ifndef CROK_CONFIG_HG__
#define CROK_CONFIG_HG__
#include <stdint.h>

// these are in game units, with each unit being a UINT32_MAX-th of the board
// diameter, including the ditch
#define CROK_DITCH_DIAM (UINT32_MAX / 24)
#define CROK_HOLE_DIAM (UINT32_MAX / 32)

#define CROK_DISCS_PER_SIDE 12
#define CROK_DISCS_TOTAL (CROK_DISCS_PER_SIDE * 2)
#define CROK_DISC_DIAM (UINT32_MAX / 18.0)
#define CROK_POST_DIAM (UINT32_MAX / 64.0)
#define CROK_NAMELEN_MAX 32
#define CROK_INPUTPACKET_MAX 24

#endif // CROK_CONFIG_HG__

// TODO: was in the middle of refactoring the board drawing code
