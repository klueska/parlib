Uthreads
==================================

To access the uthread API, include the following header file:
::

  #include <parlib/uthread.h>

Constants
------------
::

  #define UTH_EXT_BLK_MUTEX

.. c:macro:: UTH_EXT_BLK_MUTEX

  One, of possibly many in the future, reasons that a uthread has blocked
  externally.  This is required for proper implementation of the
  :c:func:`uthread_has_blocked` API call.

Types
------------
::
  
  struct uthread;
  typedef struct uthread uthread_t;

  struct schedule_ops;
  typedef struct schedule_ops schedule_ops_t;

.. c:type:: struct uthread
            uthread_t

  An opaque type used to reference and manage a user-level thread

.. c:type:: struct schedule_ops
            schedule_ops_t

  A struct containing the list of callbacks a user-level scheduler built on top
  of this uthread library needs to implement.
  :: 

    struct schedule_ops {
        void (*sched_entry)(void);
        void (*thread_runnable)(struct uthread *);
        void (*thread_paused)(struct uthread *);
        void (*thread_blockon_sysc)(struct uthread *, void *);
        void (*thread_has_blocked)(struct uthread *, int);
        void (*preempt_pending)(void);
        void (*spawn_thread)(uintptr_t pc_start, void *data);
    };

.. c:function:: void schedule_ops_t.sched_entry()



.. c:function:: void schedule_ops_t.thread_runnable(struct uthread *)



.. c:function:: void schedule_ops_t.thread_paused(struct uthread *)



.. c:function:: void schedule_ops_t.thread_blockon_sysc(struct uthread *, void *)



.. c:function:: void schedule_ops_t.thread_has_blocked(struct uthread *, int)



.. c:function:: void schedule_ops_t.preempt_pending()



.. c:function:: void schedule_ops_t.spawn_thread(uintptr_t pc_start, void *data)



External Symbols
-----------------
::

  extern struct schedule_ops *sched_ops;

.. c:var:: extern struct schedule_ops *sched_ops

  A reference to an externally defined variable which contains pointers to
  implementations of all the schedule_ops_ callbacks.

Global Variables
-----------------
::

  __thread uthread_t *current_uthread;

.. c:var:: __thread uthread_t *current_uthread



API Calls
------------
::

  int uthread_lib_init();
  void uthread_cleanup(struct uthread *uthread);
  void uthread_runnable(struct uthread *uthread);
  void uthread_yield(bool save_state, void (*yield_func)(struct uthread*, void*), void *yield_arg);
  void save_current_uthread(struct uthread *uthread);
  void highjack_current_uthread(struct uthread *uthread);
  void run_current_uthread(void);
  void run_uthread(struct uthread *uthread);
  void swap_uthreads(struct uthread *__old, struct uthread *__new);
  init_uthread_tf(uthread_t *uth, void (*entry)(void), void *stack_bottom, uint32_t size);

  #define uthread_begin_access_tls_vars(uthread)
  #define uthread_end_access_tls_vars()
  #define uthread_set_tls_var(uthread, name, val)
  #define uthread_get_tls_var(uthread, name)

.. c:function:: int uthread_lib_init()



.. c:function:: void uthread_cleanup(struct uthread *uthread)



.. c:function:: void uthread_runnable(struct uthread *uthread)



.. c:function:: void uthread_yield(bool save_state, void (*yield_func)(struct uthread*, void*), void *yield_arg)



.. c:function:: void save_current_uthread(struct uthread *uthread)



.. c:function:: void highjack_current_uthread(struct uthread *uthread)



.. c:function:: void run_current_uthread(void)

  This function does not return.

.. c:function:: void run_uthread(struct uthread *uthread)

  This function does not return.

.. c:function:: void swap_uthreads(struct uthread *__old, struct uthread *__new)



.. c:function:: init_uthread_tf(uthread_t *uth, void (*entry)(void), void *stack_bottom, uint32_t size)



.. c:function:: #define uthread_begin_access_tls_vars(uthread)



.. c:function:: #define uthread_end_access_tls_vars()



.. c:function:: #define uthread_set_tls_var(uthread, name, val)



.. c:function:: #define uthread_get_tls_var(uthread, name)



