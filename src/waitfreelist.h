#ifndef _PARLIB_WAITFREELIST_H
#define _PARLIB_WAITFREELIST_H

#include <string.h>

struct wfl_entry {
  struct wfl_entry *next;
  void *data;
};

struct wfl {
  struct wfl_entry *head;
  struct wfl_entry first;
};

#define WFL_INITIALIZER(list) {&(list).first, {0, 0}}

#ifdef __cplusplus
extern "C" {
#endif

void wfl_init(struct wfl *list);
void wfl_destroy(struct wfl *list);
size_t wfl_capacity(struct wfl *list);
size_t wfl_size(struct wfl *list);
void wfl_insert(struct wfl *list, void *data);
void *wfl_remove(struct wfl *list);

#ifdef __cplusplus
}
#endif

#endif
