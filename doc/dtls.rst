Dynamic Thread Local Storage
===============================
Dynamic thread local storage is parlib's way of providing pthread_key_create()
and pthread_set/get_specific() style sematics for uthreads and vcores. Using
dtls, different libraries can dynamically define their own set of dtls keys for
thread local storage.  Uthreads and vcores can then access the private regions
associated with these keys through the API described below.

To access the dynamic thread local storage API, include the following header file:
::

  #include <parlib/dtls.h>

Types
------------
::

  struct dtls_key;
  typedef struct dtls_key dtls_key_t;
  typedef void (*dtls_dtor_t)(void*);

.. c:type:: struct dtls_key
            dtls_key_t

  A dynamic thread local storage key. Uthreads and vcores use these keys to
  gain access to their own thread local storage region associated with the key.
  Multiple keys can be created, with each key referring to a different set of
  thread local storage regions.

.. c:type:: dtls_dtor_t

  A function pointer defining a destructor function mapped to a specific
  dtls_key.  Whenever a uthread has accessed a dtls_key that has a dtls_dtor_t
  mapped to it, the destructor function will be called before the uthread
  exits.

API Calls
------------
::

  dtls_key_t dtls_key_create(dtls_dtor_t dtor);
  void dtls_key_delete(dtls_key_t key);
  void set_dtls(dtls_key_t key, void *dtls);
  void *get_dtls(dtls_key_t key);
  void destroy_dtls();

.. c:function:: dtls_key_t dtls_key_create(dtls_dtor_t dtor)

  Initialize a dtls key for dynamically setting/getting thread local storage on
  a uthread or vcore. Takes a dtls_dtor_t as a paramameter and associates it
  with a new dtls_key_t, which gets returned.

.. c:function:: void dtls_key_delete(dtls_key_t key)

  Delete the provided dtls key.

.. c:function:: void set_dtls(dtls_key_t key, void *dtls)

  Associate a dtls storage region for the provided dtls key on the current
  uthread or vcore.

.. c:function:: void *get_dtls(dtls_key_t key)

  Get the dtls storage region previously assocaited with the provided dtls key
  on the current uthread or vcore. If no storage region has been associated
  yet, return NULL.

.. c:function:: void destroy_dtls()

  Destroy all dtls storage associated with all keys for the current uthread or
  vcore.
