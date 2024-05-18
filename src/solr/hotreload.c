#include <dlfcn.h>
#include "solr/hotreload.h"

#ifdef ENABLE_HOTRELOAD
#define HOT_FUNC(name, ...) name##_t *name;
HOT_FUNCS
#undef HOT_FUNC

const char *so_filepath = "./bin/solr.so";
static void *lib_handle = NULL;

int reload_hot_functions(void) {
	if (lib_handle != NULL) dlclose(lib_handle);

	lib_handle = dlopen(so_filepath, RTLD_NOW);
	if (lib_handle == NULL) {
		fprintf(stderr, "HOTPLUG: couldn't hot reload game loop: %s\n", dlerror());
		return 0;
	}
	#define HOT_FUNC(name, ...) \
	name = dlsym(lib_handle, #name); \
	if (name == NULL) { fprintf(stderr, \
		"HOTPLUG: couldn't find symbol %s in %s: %s", \
		#name, so_filepath, dlerror()); \
		return 0; \
	}
	HOT_FUNCS
	#undef HOT_FUNC
	return 1;
}
#endif // ENABLE_HOTRELOAD
