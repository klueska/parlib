Vcores
==================================
The vcore abstraction gives user-space an abstraction of a physical core
(within a virtual namespace) on top of which it can schedule its user-level
threads. When running code associated with a vcore, we speak of being in
**vcore context**.

Vcore context is analogous to the interrupt context in a traditional OS. Vcore
context consists of a stack, a TLS, and a set of registers, much like a basic
thread.  It is the context in which a user-level scheduler makes its decisions
and handles signals.  Once a process leaves vcore context, usually by starting
a user-level thread, any subsequent entrances to vcore context will be at the
top of the stack at :c:func:`vcore_entry`.

The vcore abstraction allows an application to manage its cores, schedule its
threads, and get the best performance it possibly can from the hardware.

To access the vcore API, include the following header file:
::

  #include <parlib/vcore.h>

Constants
------------
::

  #define LOG2_MAX_VCORES
  #define MAX_VCORES

.. c:macro:: LOG2_MAX_VCORES

  The log-base-2 of the maximum number of vcores supported on this platform

.. c:macro:: MAX_VCORES

  The maximum number of vcores supported on this platform

Types
------------
::

  struct vcore;
  typedef struct vcore vcore_t;

.. c:type:: struct vcore
            vcore_t

  An opaque type used to maintain state associated with a vcore

External Symbols
-----------------
::

  extern void vcore_entry();

.. c:function:: extern void vcore_entry()

  User defined entry point for each vcore.  If vcore_saved_ucontext_ is
  set, this function should just restore it, otherwise, it is user defined.

Global Variables
-----------------
::

  vcore_t *__vcores;
  void **__vcore_tls_descs;
  __thread ucontext_t vcore_context;
  __thread ucontext_t *vcore_saved_ucontext;
  __thread void *vcore_saved_tls_desc;

.. c:var:: vcore_t *__vcores

  An array of all of the vcores available to this application

.. c:var:: void **__vcore_tls_descs

  An array of pointers to the TLS descriptor for each vcore.

.. c:var:: __thread ucontext_t vcore_context

  Context associated with each vcore. Serves as the entry point to this vcore
  whenever the vcore is first brough up, a usercontext yields on it, or a
  signal / async I/O notification is to be handled.

.. c:var:: __thread ucontext_t *vcore_saved_ucontext

  Current user context running on each vcore, used when interrupting a
  user context because of async I/O or signal handling. Vcore 0's
  vcore_saved_ucontext_ is initialized to the continuation of the main thread's
  context the first time it's :c:func:`vcore_entry` function is invoked.

.. c:var:: __thread void *vcore_saved_tls_desc

  Current tls_desc of the user context running on each vcore, used when
  interrupting a user context because of async I/O or signal handling. Hard
  Thread 0's vcore_saved_tls_desc_ is initialized to the tls descriptor of the
  main thread's context the first time it's :c:func:`vcore_entry` function is invoked.

API Calls
------------
::

  int vcore_lib_init();
  void vcore_reenter(void (*entry_func)(void));
  int vcore_request(int k);
  void vcore_yield();
  int vcore_id(void);
  size_t num_vcores(void);
  size_t max_vcores(void);
  bool in_vcore_context();
  void clear_notif_pending(uint32_t vcoreid);
  void enable_notifs(uint32_t vcoreid);
  void disable_notifs(uint32_t vcoreid);

  #define vcore_begin_access_tls_vars(vcoreid)
  #define vcore_end_access_tls_vars()
  #define vcore_set_tls_var(name, val)
  #define vcore_get_tls_var(name)

.. c:function:: int vcore_lib_init()

  Initialization routine for the vcore subsystem. 

.. c:function:: void vcore_reenter(void (*entry_func)(void))

  Function to reenter a vcore at the top of its stack.
 
.. c:function:: int vcore_request(int k)

  Requests k additional vcores. Returns -1 if the request is impossible.
  Otherwise, blocks the calling vcore until the request is granted and returns
  0.

.. c:function:: void vcore_yield()

  Relinquishes the calling vcore.

.. c:function:: int vcore_id(void)

  Returns the id of the calling vcore.

.. c:function:: size_t num_vcores(void)

  Returns the current number of vcores allocated.

.. c:function:: size_t max_vcores(void)

  Returns the maximum number of allocatable vcores.

.. c:function:: bool in_vcore_context()

  Returns whether you are currently running in vcore context or not.

.. c:function:: void clear_notif_pending(uint32_t vcoreid)

  Clears the flag for pending notifications

.. c:function:: void enable_notifs(uint32_t vcoreid)

  Enable Notifications

.. c:function:: void disable_notifs(uint32_t vcoreid)

  Disable Notifications

.. c:function:: #define vcore_begin_access_tls_vars(vcoreid)

  Begin accessing TLS variables associated with a specific vcore (possibly a
  different from the current one). Matched one-to-one with a following call to
  :c:func:`vcore_end_access_tls_vars` within the same function.

.. c:function:: #define vcore_end_access_tls_vars()

  End access to a vcore's TLS variables. Matched one-to-one with a previous
  call to :c:func:`vcore_begin_access_tls_vars` within the same function.

.. c:function:: #define vcore_set_tls_var(name, val)

  Set a single variable in the TLS of the current vcore. Mostly useful when
  running in uthread context and want to set something vcore specific.

.. c:function:: #define vcore_get_tls_var(name)

  Get a single variable from the TLS of the current vcore. Mostly useful when
  running in uthread context and want to get something vcore specific.

