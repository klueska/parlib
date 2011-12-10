# Add the m4 directory to the include path for aclocal
ACLOCAL_AMFLAGS=-I m4

# Set up global variables
LIBNAME = parlib
CAP_LIBNAME = $(shell echo $(LIBNAME) | sed -e 's/^[a-z]/\U&/g')
SRCDIR = $(srcdir)/@SRCDIR@

# List of C FILES to build into objects
LIB_CFILES = \
  @SRCDIR@/spinlock.c \
  @SRCDIR@/mcs.c      \
  @SRCDIR@/tls.c      \
  @SRCDIR@/uthread.c  \
  @SRCDIR@/vcore.c

LIB_HFILES = \
  @SRCDIR@/spinlock.h  \
  @SRCDIR@/parlib.h    \
  @SRCDIR@/mcs.h       \
  @SRCDIR@/queue.h     \
  @SRCDIR@/tls.h       \
  @SRCDIR@/uthread.h   \
  @SRCDIR@/vcore.h

LIB_SFILES = 

LIB_INTERNAL_FILES = \
  @SRCDIR@/internal/futex.h \
  @SRCDIR@/internal/glibc-tls.h \
  @SRCDIR@/internal/tls.h \
  @SRCDIR@/internal/vcore.h

if ARCH_i686
SYSDEPDIR = $(srcdir)/@SYSDEPDIR_i686@

LIB_SFILES += 

LIB_HFILES += \
  @SYSDEPDIR_i686@/arch.h   \
  @SYSDEPDIR_i686@/atomic.h

endif

if ARCH_x86_64
SYSDEPDIR = $(srcdir)/@SYSDEPDIR_x86_64@

LIB_SFILES += \
  @SYSDEPDIR_x86_64@/swapcontext.S \
  @SYSDEPDIR_x86_64@/setcontext.S  \
  @SYSDEPDIR_x86_64@/getcontext.S

LIB_HFILES += \
  @SYSDEPDIR_x86_64@/arch.h   \
  @SYSDEPDIR_x86_64@/atomic.h

LIB_INTERNAL_FILES += \
  @SYSDEPDIR_x86_64@/internal/ucontext_i.h

endif
LIB_FILES = $(LIB_CFILES) $(LIB_SFILES) $(LIB_HFILES) $(LIB_INTERNAL_FILES)

# Setup parameters to build the library
lib_LTLIBRARIES = libparlib.la
libparlib_la_CFLAGS = $(AM_CFLAGS)
libparlib_la_CFLAGS += -I$(SYSDEPDIR)
libparlib_la_SOURCES = $(LIB_FILES)
libparlib_la_LIBADD = -lpthread
if STATIC_ONLY
libparlib_la_LDFLAGS = -all-static
endif

# Setup a directory where all of the include files will be installed
parlibincdir = $(includedir)/$(LIBNAME)
dist_parlibinc_DATA = $(LIB_HFILES)

# Setup parameters to build the test programs
check_PROGRAMS = lock_test vcore_test

lock_test_SOURCES =  @TESTSDIR@/lock_test.c
lock_test_CFLAGS = $(AM_CFLAGS)
lock_test_CFLAGS += -I$(SRCDIR) -I$(SYSDEPDIR)
lock_test_LDFLAGS = -all-static
lock_test_LDADD = libparlib.la

vcore_test_SOURCES = @TESTSDIR@/vcore_test.c
vcore_test_CFLAGS = $(AM_CFLAGS)
vcore_test_CFLAGS += -I$(SRCDIR) -I$(SYSDEPDIR)
vcore_test_LDFLAGS = -all-static
vcore_test_LDADD = libparlib.la 

man_MANS = \
  doc/man/$(LIBNAME).1

# Don't suppport doing things this way for now because automake is dumb and
# doesn't allow 'generated' texi files...
# Instead we'll add the generated .info file and install it manually...
#info_TEXINFOS = \
#  doc/texinfo/$(CAP_LIBNAME).texi

# Include pre-generated documentation in the distribution
EXTRA_DIST = \
  doc/html \
  doc/singlehtml \
  doc/man \
  doc/texinfo/$(CAP_LIBNAME).info \
  doc/latex/$(CAP_LIBNAME).pdf

$(abs_builddir)/doc/%: | $(abs_builddir)/doc
	cd $(srcdir)/doc; \
	if [ -f Makefile ]; then \
	  make BUILDDIR=$(abs_builddir)/doc $(@F); \
	else \
	  cp -R $(@F) $(@D); \
	fi;

$(abs_builddir)/doc/texinfo/$(CAP_LIBNAME).info: | $(abs_builddir)/doc
	cd $(srcdir)/doc; \
	if [ -f Makefile ]; then \
	  make BUILDDIR=$(abs_builddir)/doc info; \
	else \
	  cp -R texinfo $(@D); \
	fi;

$(abs_builddir)/doc/latex/$(CAP_LIBNAME).pdf: | $(abs_builddir)/doc
	cd $(srcdir)/doc; \
	if [ -f Makefile ]; then \
	  make BUILDDIR=$(abs_builddir)/doc latexpdf; \
	else \
	  cp -R latex $(@D); \
	fi;

$(abs_builddir)/doc:
	mkdir -p $@

doc/%: $(abs_builddir)/doc/%
doc/man/$(LIBNAME).1: $(abs_builddir)/doc/man
doc/texinfo/$(CAP_LIBNAME).info: $(abs_builddir)/doc/texinfo/$(CAP_LIBNAME).info
doc/latex/$(CAP_LIBNAME).pdf: $(abs_builddir)/doc/latex/$(CAP_LIBNAME).pdf

doc: \
  $(abs_builddir)/doc/html \
  $(abs_builddir)/doc/singlehtml \
  $(abs_builddir)/doc/man \
  $(abs_builddir)/doc/texinfo/$(CAP_LIBNAME).info \
  $(abs_builddir)/doc/latex/$(CAP_LIBNAME).pdf

ChangeLog:
	cd $(srcdir); \
	if [ -f git-changelog.sh ]; then \
	  ./git-changelog.sh; \
	fi;

clean-local:
	rm -rf doc

install-data-hook: doc
	install -d $(docdir)
	install -d $(infodir)
	cp -r $(abs_builddir)/doc/html $(docdir)
	cp -r $(abs_builddir)/doc/singlehtml $(docdir)
	cp -r $(abs_builddir)/doc/texinfo/$(CAP_LIBNAME).info $(infodir)
	cp -r $(abs_builddir)/doc/latex/$(CAP_LIBNAME).pdf $(docdir)

uninstall-hook:
	rm -rf $(litheincdir)
	rm -rf $(docdir)
	rm -rf $(infodir)/$(CAP_LIBNAME).info
.PHONY: ChangeLog

