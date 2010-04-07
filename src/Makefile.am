## Process this file with automake to produce Makefile.in

INCLUDES = \
    -DDATADIR=\""$(datadir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gweled"\" \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gweled

gweled_SOURCES = \
	main.c \
	board_engine.c board_engine.h \
	graphic_engine.c graphic_engine.h \
	sge_core.c sge_core.h \
	sge_utils.c sge_utils.h \
	music.c music.h \
	callbacks.c callbacks.h

EXTRA_DIST =			\
	$(Data_DATA)

Datadir = $(datadir)/gweled/
Data_DATA = 	\
	gweled.glade \
	gweled_icon.png \
	gweled_logo.png

gweled_LDADD = @PACKAGE_LIBS@

