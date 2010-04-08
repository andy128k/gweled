## Process this file with automake to produce Makefile.in

INCLUDES = \
    -DDATADIR=\""$(datadir)"\"  \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gweled"\"  \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"

AM_CFLAGS = 		  \
    @GTK_CFLAGS@      \
	@RSVG_CFLAGS@	  \
	@GNOME_CFLAGS@    \
	@MIKMOD_CFLAGS@   \
	@PTHREAD_CFLAGS@

bin_PROGRAMS = gweled

gweled_SOURCES = \
	main.c \
	board_engine.c board_engine.h \
	graphic_engine.c graphic_engine.h \
	sge_core.c sge_core.h \
	sge_utils.c sge_utils.h \
	music.c music.h \
	callbacks.c callbacks.h

gweled_LDADD = @GTK_LIBS@ @RSVG_LIBS@ @GNOME_LIBS@ @MIKMOD_LIBS@ @PTHREAD_LIBS@


