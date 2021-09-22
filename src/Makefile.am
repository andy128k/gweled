## Process this file with automake to produce Makefile.in

gweled_CFLAGS =           \
    @GTK_CFLAGS@          \
	@RSVG_CFLAGS@	      \
	@CANBERRA_CFLAGS@     \
	@CANBERRA_GTK_CFLAGS@ \
	-DDATADIR=\""$(datadir)"\"                    \
    -DDATADIR=\""$(datadir)"\"                    \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gweled"\"    \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"  \
	-DSCOREDIR=\""$(scoredir)"\"                  \
	$(NULL)

bin_PROGRAMS = gweled

gweled_SOURCES = \
	main.c main.h \
	callbacks.c \
	board_engine.c board_engine.h \
	graphic_engine.c graphic_engine.h \
	sge_core.c sge_core.h \
	sge_utils.c sge_utils.h \
	sound.c sound.h \
	games-score.c games-score.h \
	games-scores.c games-scores.h \
	games-scores-backend.c games-scores-backend.h \
	games-scores-dialog.c games-scores-dialog.h \
	games-setgid-io.c games-setgid-io.h \
	games-scores-dialog-private.h

gweled_LDADD = @GTK_LIBS@ @RSVG_LIBS@ @CANBERRA_LIBS@ @CANBERRA_GTK_LIBS@

CLEANFILES =            \
    $(BUILT_SOURCES)    \
    *~                  \
    $(NULL)


