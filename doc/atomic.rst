Atomic Memory Operations
==================================
To access the atomic memory operations  API, include the following header file:
::

  #include <parlib/atomic.h>

Types
------------
::

  typedef void* atomic_t;

.. c:type:: atomic_t

API Calls
------------
::

  void atomic_init(atomic_t *number, long val);
  void *atomic_swap_ptr(void **addr, void *val);
  long atomic_swap(atomic_t *addr, long val);
  uint32_t atomic_swap_u32(uint32_t *addr, uint32_t val);

  #define mb()
  #define cmb()
  #define rmb()
  #define wmb()
  #define wrmb()
  #define rwmb()
  #define mb_f()
  #define rmb_f()
  #define wmb_f()
  #define wrmb_f()
  #define rwmb_f()

.. c:function:: void atomic_init(atomic_t *number, long val)
.. c:function:: void *atomic_swap_ptr(void **addr, void *val)
.. c:function:: long atomic_swap(atomic_t *addr, long val)
.. c:function:: uint32_t atomic_swap_u32(uint32_t *addr, uint32_t val)
.. c:function:: #define mb()
.. c:function:: #define cmb()
.. c:function:: #define rmb()
.. c:function:: #define wmb()
.. c:function:: #define wrmb()
.. c:function:: #define rwmb()
.. c:function:: #define mb_f()
.. c:function:: #define rmb_f()
.. c:function:: #define wmb_f()
.. c:function:: #define wrmb_f()
.. c:function:: #define rwmb_f()
