MCS Locks
==================================

.. c:type:: struct mcs_lock_qnode
.. c:type:: struct mcs_lock
.. c:type:: mcs_dissem_flags_t
.. c:type:: mcs_barrier_t
.. c:function:: int mcs_barrier_init(mcs_barrier_t* b, size_t nprocs)
.. c:function:: void mcs_barrier_wait(mcs_barrier_t* b, size_t vcoreid)
.. c:function:: void mcs_lock_init(struct mcs_lock *lock)
.. c:function:: void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)
.. c:function:: void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)
.. c:function:: void mcs_lock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)
.. c:function:: void mcs_unlock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)

