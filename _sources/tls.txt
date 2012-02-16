Thread Local Storage
==================================

.. c:var:: extern void *main_tls_desc
.. c:var:: extern __thread void *current_tls_desc
.. c:function:: void *allocate_tls(void)
.. c:function:: void *reinit_tls(void *tcb)
.. c:function:: void free_tls(void *tcb)
.. c:function:: void set_tls_desc(void *tls_desc, uint32_t vcoreid)
.. c:function:: void *get_tls_desc(uint32_t vcoreid)
