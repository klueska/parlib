Memory Pools
==================================
Memory pools are designed for efficient dynamic memory management when 
you have a fixed number of fixed size objects you need to manage. A large
chunk of memory must first be allocated by traditional means before passing
it to the pool API.  Once received however, the memory is managed according to
the restrictions described above, without worrying about fragmentation or other
traditional dynamic memory management concerns.

To access the pool API, include the following header file:
::

  #include <parlib/pool.h>

Types
------------
::

  struct pool;
  typedef struct pool pool_t;

.. c:type:: struct pool
            pool_t

  Opaque type used to reference and manage a pool

API Calls
------------
::

  void pool_init(pool_t *pool, void* buffer, void **object_queue,
                 size_t num_objects, size_t object_size);
  size_t pool_size(pool_t *pool);
  size_t pool_available(pool_t *pool);
  void* pool_alloc(pool_t *pool);
  int pool_free(pool_t* pool, void *object);

.. c:function:: void pool_init(pool_t *pool, void* buffer, void **object_queue, size_t num_objects, size_t object_size)

  Initialize a pool.  All memory MUST be allocated externally.  The pool
  implementation simply manages the objects contained in the buffer passed to
  this function.  This allows us to use the same pool implementation for both
  statically and dynamically allocated memory.

.. c:function:: size_t pool_size(pool_t *pool)

  Check how many objects the pool is able to hold 

.. c:function:: size_t pool_available(pool_t *pool)

  See how many objects are currently available for allocation from the pool.

.. c:function:: void* pool_alloc(pool_t *pool)

  Get an object from the pool

.. c:function:: int pool_free(pool_t* pool, void *object)

  Put an object into the pool

