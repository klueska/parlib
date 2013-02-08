Thread Local Storage
==================================

To access the thread local storage API, include the following header file:
::

  #include <parlib/tls.h>

Global Variables
-----------------
::

  void *main_tls_desc;
  __thread void *current_tls_desc;

.. c:var:: void *main_tls_desc
.. c:var:: __thread void *current_tls_desc

API Calls
------------
::

  void *allocate_tls(void);
  void *reinit_tls(void *tcb);
  void free_tls(void *tcb);
  void set_tls_desc(void *tls_desc, uint32_t vcoreid);
  void *get_tls_desc(uint32_t vcoreid);
  
.. c:function:: void *allocate_tls(void)
.. c:function:: void *reinit_tls(void *tcb)
.. c:function:: void free_tls(void *tcb)
.. c:function:: void set_tls_desc(void *tls_desc, uint32_t vcoreid)
.. c:function:: void *get_tls_desc(uint32_t vcoreid)
