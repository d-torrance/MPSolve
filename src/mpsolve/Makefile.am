NULL = 

bin_PROGRAMS = mpsolve

man_MANS = ${builddir}/mpsolve.1

EXTRA_DIST = mpsolve.1

mpsolve_CFLAGS = \
        -I${top_srcdir}/include \
	-I. \
	-I${top_builddir}/include \
	$(GMP_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(NULL)

mpsolve_SOURCES = \
	iteration-logger.h \
	iteration-logger.c \
	mpsolve.c \
	$(NULL)

mpsolve_LDFLAGS = 

mpsolve_LDADD = \
	${top_builddir}/src/libmps/libmps.la \
	$(GMP_LIBS) \
	$(GTK_LIBS)

nodist_EXTRA_mpsolve_SOURCES = dummy.cxx
