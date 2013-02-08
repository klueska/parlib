.. Parlib documentation master file, created by
   sphinx-quickstart on Thu Dec  1 21:19:28 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. toctree::
  :maxdepth: 2

Welcome to Parlib's API Reference!
==================================

Parlib is a library meant to ease the development of software written for
highly parallel systems.  It was originally written for the Akaros operating
system, and has since been ported to Linux.  It was originally designed as an
emulation layer on top of Linux that would allow developers to write
applications and test them on a linux system before deplying them on Akaros.
Since then, however, Parlib has proved itself useful in its own right as a
standalone libary for Linux.  Most notably, as the backend for the Lithe
implementation.

The official Home Page for Parlib is: 
`<http://akaros.cs.berkeley.edu/parlib>`_.

At present, Parlib provides 6 primary services to developers:

1. A user-level abstraction for managing physical cores in a virtualized-namespace (vcores)
2. A user-level thread abstraction (uthreads)
3. User-space MCS locks, MCS barriers, spinlocks and spinbarriers
4. Static and dynamic user-level thread local storage
5. Pool and slab allocator dynamic memory management abstractions
6. A standardized API for common atomic memory operations

