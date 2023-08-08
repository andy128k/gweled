gweled_CFLAGS =           \
    @GTK_CFLAGS@          \
    @CLUTTER_CFLAGS@	  \
    @CLUTTER_GTK_CFLAGS@  \
    @CANBERRA_CFLAGS@     \
    @CANBERRA_GTK_CFLAGS@ \
    @GNOME_GAMES_CFLAGS@  \
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
	board_engine.c board_engine.h \
	graphic_engine.c graphic_engine.h \
	sge_core.c sge_core.h \
	sge_utils.c sge_utils.h \
	sound.c sound.h \
	gweled-scores.c gweled-scores.h \
	$(RESOURCES) \
	$(NULL)
	
	
gweled_LDADD = @GTK_LIBS@ @CLUTTER_LIBS@ @CLUTTER_GTK_LIBS@ @CANBERRA_LIBS@ @CANBERRA_GTK_LIBS@ @GNOME_GAMES_LIBS@ -lm

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


