#ifndef MAHJCELL_NET
#define MAHJCELL_NET
#include "solr.h"

// wraps common cases, and leaves custom
// open with 0th (invalid) kind, where it
// only handles tick and crc busywork
ssize_t mahjcell_send(solr *, int kind);
ssize_t mahjcell_broadcast(solr *s, int kind);

#endif
