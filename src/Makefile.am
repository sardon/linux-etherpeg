## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = etherpeg

etherpeg_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	promiscuity.c promiscuity.h \
	sort_frame.c sort_frame.h

etherpeg_LDADD = @GTK_LIBS@

