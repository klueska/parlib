Uthreads
==================================

.. c:type:: struct uthread
.. c:type:: uthread_t
.. c:type:: extern __thread uthread_t *current_uthread
.. c:type:: struct schedule_ops
.. c:var:: extern struct schedule_ops *sched_ops
.. c:function:: extern void uthread_vcore_entry()
.. c:function:: int uthread_lib_init()
.. c:function:: void uthread_init(struct uthread *uth)
.. c:function:: void uthread_cleanup(struct uthread *uthread)
.. c:function:: void uthread_runnable(struct uthread *uthread)
.. c:function:: void uthread_yield(bool save_state)
.. c:function:: bool check_preempt_pending(uint32_t vcoreid)
.. c:function:: void save_current_uthread(struct uthread *uthread)
.. c:function:: void set_current_uthread(struct uthread *uthread)
.. c:function:: void run_current_uthread(void) __attribute((noreturn))
.. c:function:: void run_uthread(struct uthread *uthread) __attribute((noreturn))
.. c:function:: void swap_uthreads(struct uthread *__old, struct uthread *__new)
.. c:function:: void init_uthread_tf(uthread_t *uth, void (*entry)(void), void *stack_bottom, uint32_t size)
.. c:function:: #define uthread_set_tls_var(uthread, name, val)
.. c:function:: #define uthread_get_tls_var(uthread, name)
