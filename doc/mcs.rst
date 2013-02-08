MCS Locks
==================================
MCS locks are a spinlock style lock designed for more efficient execution on
multicore processors. They are designed to mitigate cache clobbering and TLB
shootdowns by having each call site spin on a core-local lock variable, rather
than the single lock variable used by traditional spinlocks. These locks should
really only be used while running in vcore context.

To access the mcs lock API, include the following header file:
::

  #include <parlib/mcs.h>

Constants
------------
::

  #define MCS_LOCK_INIT
  #define MCS_QNODE_INIT

.. c:macro:: MCS_LOCK_INIT

  Static initializer for an mcs_lock_t_

.. c:macro:: MCS_QNODE_INIT

  Static initializer for an mcs_lock_qnode_t_

Types
------------
::

  struct mcs_lock_qnode;
  typedef struct mcs_lock_qnode mcs_lock_qnode_t;

  struct mcs_lock;
  typedef struct mcs_lock mcs_lock_t;

  struct mcs_dissem_flags;
  typedef struct mcs_dissem_flags mcs_dissem_flags_t;

  struct mcs_barrier;
  typedef struct mcs_barrier mcs_barrier_t;

.. c:type:: struct mcs_lock_qnode
            mcs_lock_qnode_t

  An MCS lock qnode.  MCS locks are maintained as a queue of MCS qnode
  pointers, and locks are granted in order of request, using the qnode
  pointer passed at each :c:func:`mcs_lock_lock` call.

.. c:type:: struct mcs_lock
            mcs_lock_t

  An MCS lock itself. This data type keeps track of whether the lock is
  currently held or not, as well as the list of qnode pointers described above.

.. c:type:: struct mcs_dissem_flags
            mcs_dissem_flags_t

  Dissemmenation flags used by the MCS barriers described below. This data type
  should never normally be accessed by an external library.  They are used
  internally by the MCS barrier implementation, but exist as part of the API
  because the mcs_barrier struct contains them.

.. c:type:: struct mcs_barrier
            mcs_barrier_t

  An MCS barrier. This data type is used to reference an MCS barrier across the
  various MCS barrier API Calls.

API Calls
------------
::

  void mcs_lock_init(struct mcs_lock *lock);
  void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
  void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
  void mcs_lock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
  void mcs_unlock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
  void mcs_barrier_init(mcs_barrier_t* b, size_t num_vcores);
  void mcs_barrier_wait(mcs_barrier_t* b, size_t vcoreid);

.. c:function:: void mcs_lock_init(struct mcs_lock *lock)

  Initializes an MCS lock.

.. c:function:: void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)

  Locks an MCS lock, associating a call-site specific qnode with the lock in the process.

.. c:function:: void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)

  Unlocks an MCS lock, releasing the call-site specific qnode associated with
  the current lock holder.

.. c:function:: void mcs_lock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)

  A signal-safe implementation of :c:func:`mcs_lock_lock`.  While the lock is held, the
  lockholder will not be interrrupted to run any signal handlers.

.. c:function:: void mcs_unlock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)

  The :c:func:`mcs_lock_unlock` counterpart to :c:func:`mcs_lock_notifsafe`. After releasing
  the lock, signals may be processed again.

.. c:function:: void mcs_barrier_init(mcs_barrier_t* b, size_t num_vcores)

  Initializes an MCS barrier with the number of vcores associated with the barrier.

.. c:function:: void mcs_barrier_wait(mcs_barrier_t* b, size_t vcoreid)

  Waits on an MCS barrier for the specified vcoreid.

