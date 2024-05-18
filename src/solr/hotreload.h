#ifndef HOTRELOAD_HG__
#define HOTRELOAD_HG__

#include "solr/solr.h"

#ifdef ENABLE_HOTRELOAD
#define HOT_FUNC(name, ret, ...) \
	typedef ret (name##_t)(__VA_ARGS__); \
  extern name##_t *name;
HOT_FUNCS
#undef HOT_FUNC

int reload_hot_functions(void);
#endif // ENABLE_HOTRELOAD
#endif // HOTRELOAD_HG__

