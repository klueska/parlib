Vcores
==================================

.. c:var:: struct vcore
.. c:var:: extern struct vcore *__vcores
.. c:var:: extern void **vcore_tls_descs
.. c:var:: extern __thread ucontext_t vcore_context
.. c:var:: extern __thread ucontext_t *vcore_saved_ucontext
.. c:var:: extern __thread void *vcore_saved_tls_desc
.. c:function:: extern void vcore_entry() __attribute__((weak))
.. c:function:: extern int vcore_lib_init();
.. c:function:: int vcore_request(int k);
.. c:function:: void vcore_yield();
.. c:function:: int vcore_id(void)
.. c:function:: size_t num_vcores(void)
.. c:function:: size_t max_vcores(void)
.. c:function:: size_t limit_vcores(void)
.. c:function:: bool in_vcore_context()
.. c:function:: void clear_notif_pending(uint32_t vcoreid);
.. c:function:: void enable_notifs(uint32_t vcoreid);
.. c:function:: void disable_notifs(uint32_t vcoreid);
.. c:function:: #define vcore_set_tls_var(name, val)
.. c:function:: #define vcore_get_tls_var(name)
