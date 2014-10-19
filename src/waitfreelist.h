#ifndef _PARLIB_WAITFREELIST_H
#define _PARLIB_WAITFREELIST_H

#include <stdbool.h>
#include <string.h>

struct wfl_slot {
  struct wfl_slot *next;
  void *data;
};

struct wfl {
  struct wfl_slot *head;
  size_t slot_size;
  size_t size;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COMPILING_PARLIB
# define wfl_init INTERNAL(wfl_init)
# define wfl_init_ss INTERNAL(wfl_init_ss)
# define wfl_cleanup INTERNAL(wfl_cleanup)
# define wfl_insert INTERNAL(wfl_insert)
# define wfl_insert_into INTERNAL(wfl_insert_into)
# define wfl_remove INTERNAL(wfl_remove)
# define wfl_remove_from INTERNAL(wfl_remove_from)
# define wfl_remove_all INTERNAL(wfl_remove_all)
# define wfl_capacity INTERNAL(wfl_capacity)
# define wfl_size INTERNAL(wfl_size)
#endif

/* Initialize a WFL. Memory for the wfl struct must be allocated externally. */
void wfl_init(struct wfl *list);
void wfl_init_ss(struct wfl *list, size_t slot_size);

/* Cleanup a WFL. Memory for the wfl struct must be freed externally. */
void wfl_cleanup(struct wfl *list);

/* Insert an item into a WFL. A pointer to the slot where the data is stored in
 * the WFL is returned. This function will never fail. */
struct wfl_slot *wfl_insert(struct wfl *list, void *data);

/* Try to insert an item into a specific slot in a WFL. If the slot is already
 * occupied, return false, indicating a failure. Otherwise return true. */
bool wfl_insert_into(struct wfl *list, struct wfl_slot *slot, void *data);

/* Try to remove an item from the WFL. If an item is found, return it. If an
 * item is not found, return NULL. Removals are not synchronized with
 * insertions (meaning just because we return NULL, doesn't mean the list is
 * empty if an insertion was happening concurrently). */
void *wfl_remove(struct wfl *list);

/* Try to remove an item from a specific slot in a WFL. If the slot is empty,
 * return NULL. This call is also not synchronized with insertions and may not
 * remove an item if an insertion happens concurrently. */
void *wfl_remove_from(struct wfl *list, struct wfl_slot *slot);

/* Try to remove all items from a WFL. Return the number of items removed.
 * Just like the other remove variants, this call is also not synchronized with
 * insertions, so it may not actually remove everything from the list if
 * insertions are happening concurrently. */
size_t wfl_remove_all(struct wfl *list, void *data);

/* Return the current capacity of a WFL (i.e. how many slots have been
 * allocated for it). This call is not synchronized with either insertion or
 * removal, so it is just an estimate of the current capacity. */
size_t wfl_capacity(struct wfl *list);

/* Return the current size of the WFL (i.e. how many items are currently
 * present in). This call is not synchronized with either insertion or
 * removal, so it is just an estimate of the current size . */
size_t wfl_size(struct wfl *list);

/* Iterate through all items in a WFL. Not synchronized with insertions or
 * removals, so care must be taken by the caller to ensure the integrity of the
 * items being operated on. */
#define wfl_foreach_unsafe(elm, list) \
  for (struct wfl_slot *_p = (list)->head; \
       elm = _p == NULL ? NULL : _p->data, _p != NULL; \
       _p = _p->next) \
    if (elm)

#ifdef __cplusplus
}
#endif

#endif
