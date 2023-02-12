#ifndef MEMUTIL_HG__
#define MEMUTIL_HG__

struct sarr {
  size_t len;
  size_t elem_s;
  void *head;
};

typedef void *(*malloc_fun)(size_t, const char *);
struct sarr sarr_alloc_with(size_t elem_s, size_t s, malloc_fun f,
                            const char *why);
struct sarr sarr_alloc(size_t elem_s, size_t s, const char *why);
void sarr_free(struct sarr a);
void *safe_malloc(size_t s, const char *why);
void *safe_calloc(size_t s, const char *why);

#endif // MEMUTIL_HG__
