Slab Memory Allocator
==================================

To access the slab allocator API, include the following header file:
::

  #include <parlib/slab.h>

Types
------------
::

  struct slab_cache;
  typedef struct slab_cache slab_cache_t;

  typedef void (*slab_cache_ctor_t)(void *, size_t);
  typedef void (*slab_cache_dtor_t)(void *, size_t);

.. c:type:: struct slab_cache
            slab_cache_t

.. c:type:: slab_cache_ctor_t

.. c:type:: slab_cache_dtor_t

API Calls
------------
::

  struct slab_cache *slab_cache_create(const char *name, size_t obj_size,
                                       int align, int flags, 
                                       slab_cache_ctor_t ctor,
                                       slab_cache_dtor_t dtor);
  void slab_cache_destroy(struct slab_cache *cp);
  void *slab_cache_alloc(struct slab_cache *cp, int flags);
  void slab_cache_free(struct slab_cache *cp, void *buf);

.. c:function:: struct slab_cache *slab_cache_create(const char *name, size_t obj_size, int align, int flags, slab_cache_ctor_t ctor, slab_cache_dtor_t dtor)



.. c:function:: void slab_cache_destroy(struct slab_cache *cp)



.. c:function:: void *slab_cache_alloc(struct slab_cache *cp, int flags)



.. c:function:: void slab_cache_free(struct slab_cache *cp, void *buf)



