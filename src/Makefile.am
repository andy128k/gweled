gweled_CFLAGS =           \
    @GTK_CFLAGS@          \
	@RSVG_CFLAGS@	      \
	@CANBERRA_CFLAGS@     \
	@CANBERRA_GTK_CFLAGS@ \
    -DDATADIR=\""$(datadir)"\"                    \
    -DPREFIX=\""$(prefix)"\"                      \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gweled"\"    \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"  \
	-DSCOREDIR=\""$(scoredir)"\"                  \
	$(NULL)

bin_PROGRAMS = gweled

RESOURCES =                \
    gweled-resources.c     \
    gweled-resources.h     \
    $(NULL)


gweled_SOURCES = \
	main.c main.h \
	gweled-gui.c gweled-gui.h \
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
	games-scores-dialog-private.h \
	$(RESOURCES) \
	$(NULL)
	
	
gweled_LDADD = @GTK_LIBS@ @RSVG_LIBS@ @CANBERRA_LIBS@ @CANBERRA_GTK_LIBS@ -lm

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gweled.gresource.xml)

gweled-resources.h gweled-resources.c: gweled.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name gweled $(srcdir)/gweled.gresource.xml


EXTRA_DIST =               \
    gweled.gresource.xml   \
    gweled.ui              \
    gweled-headermenu.ui   \
    $(NULL)


BUILT_SOURCES = 		\
	$(RESOURCES)		\
	$(NULL)


CLEANFILES =            \
    $(BUILT_SOURCES)    \
    *~                  \
    $(NULL)


