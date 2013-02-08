Spinlocks
==================================

To access the spinlock API, include the following header file:
::

  #include <parlib/spinlock.h>

Constants
------------
::

  #define SPINLOCK_INITIALIZER

.. c:macro SPINLOCK_INITIALIZER

  Static initializer for a spinlock_t_

Types
------------
::

  struct spinlock;
  typedef struct spinlock spinlock_t;

.. c:type:: struct spinlock 
            spinlock_t

API Calls
------------
::

  void spinlock_init(spinlock_t *lock);
  int spinlock_trylock(spinlock_t *lock);
  void spinlock_lock(spinlock_t *lock);
  void spinlock_unlock(spinlock_t *lock);

.. c:function:: void spinlock_init(spinlock_t *lock)
.. c:function:: int spinlock_trylock(spinlock_t *lock)
.. c:function:: void spinlock_lock(spinlock_t *lock)
.. c:function:: void spinlock_unlock(spinlock_t *lock)

