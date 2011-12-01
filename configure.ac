# Require a minimum autoconf version
AC_PREREQ([2.65])

# Initialize autoconf
# Specify package's name, version number, and bug-report address.
AC_INIT([Parlib], [1.0], [klueska@cs.berkeley.edu])

# Directory containing any external m4 macros
AC_CONFIG_MACRO_DIR([m4])

# Auxiliary scripts such as install-sh and depcomp should be in DIRECTORY
AC_CONFIG_AUX_DIR([build-aux])

# Initialize automake
# Turn on all Automake warnings and report them as errors.
# This is a foreign package
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror -Wno-portability])

# Checks for a C compiler
AC_PROG_CC

# Checks for a C++ compiler
#AC_PROG_CXX

# Check for an assembler
AM_PROG_AS

# Checks for libtool
AC_PROG_LIBTOOL

# Check for HEADERS and #define HAVE_HEADER_H for each header found
#AC_CHECK_HEADERS([HEADERS ...])

# Output the following to config.h 
#AC_DEFINE(VARIABLE, VALUE, DESCRIPTION)

# Declare config.h as the output header
AC_CONFIG_HEADERS([config.h])

# Declare Makefiles as output files
AC_CONFIG_FILES([
  Makefile 
])

# A safety check. FILE is a distributed source file, making sure that
# 'configure' is not run from outerspace.
AC_CONFIG_SRCDIR([src/vcore.c])

# Set the common AM_CFLAGS for all Makefile.am files
AC_SUBST([AM_CFLAGS],["    \
  -g -O2 -Wall -std=gnu99  \
  -Wno-unused-function     \
  -Wno-unused-value        \
  -Wno-missing-braces 
"])

# Set up some global variables for use in the makefile
SRCDIR=src
TESTSDIR=tests
SYSDEPDIR_BASE=$SRCDIR/sysdeps/unix/sysv/linux
SYSDEPDIR_i686=$SYSDEPDIR_BASE/i686
SYSDEPDIR_x86_64=$SYSDEPDIR_BASE/x86_64
AC_SUBST([SRCDIR])
AC_SUBST([TESTSDIR])
AC_SUBST([SYSDEPDIR_i686])
AC_SUBST([SYSDEPDIR_x86_64])
AM_SUBST_NOTMAKE([SRCDIR])
AM_SUBST_NOTMAKE([TESTSDIR])
AM_SUBST_NOTMAKE([SYSDEPDIR_i686])
AM_SUBST_NOTMAKE([SYSDEPDIR_x86_64])

case $host in
  *i686*)   
            ARCH_i686=true;;
  *x86_64*) 
            ARCH_x86_64=true;;
esac
AM_CONDITIONAL([ARCH_i686], [test x$ARCH_i686 = xtrue])
AM_CONDITIONAL([ARCH_x86_64], [test x$ARCH_x86_64 = xtrue])

# Check whether LIBRARY exists and contains FUNCT
# Execute ACT-IF-FOUND if it does.  ACT-IF-NOT otherwise.
#AC_CHECK_LIB(LIBRARY, FUNCT, [ACT-IF-FOUND], [ACT-IF-NOT])

# Actually output all declared files
AC_OUTPUT
