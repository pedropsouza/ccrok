#ifndef HOTRELOAD_HG__
#define HOTRELOAD_HG__

#include "solr/solr.h"

#define HOT_FUNC(name, ...) extern name##_t *name;
HOT_FUNCS
#undef HOT_FUNC

int reload_hot_functions(void);

#endif // HOTRELOAD_HG__

