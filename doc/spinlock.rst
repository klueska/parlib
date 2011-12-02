Spinlocks
==================================

.. c:type:: spinlock_t
.. c:function:: void spinlock_init(spinlock_t *lock)
.. c:function:: int spinlock_trylock(spinlock_t *lock)
.. c:function:: void spinlock_lock(spinlock_t *lock)
.. c:function:: void spinlock_unlock(spinlock_t *lock)
