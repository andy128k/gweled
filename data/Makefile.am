icons = \
    apps_16x16_gweled.png \
    apps_22x22_gweled.png \
    apps_24x24_gweled.png \
    apps_48x48_gweled.png \
    apps_64x64_gweled.png \
    apps_scalable_gweled.svg

# .desktop file
DESKTOP_IN_FILES= gweled.desktop.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

$(desktop_DATA): %.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@


# AppData
appdatadir = $(datadir)/appdata
appdata_in_files = gweled.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

# GSettings schema
gsettingsdir = $(datadir)/glib-2.0/schemas
gsettings_SCHEMAS = org.gweled.gweled.gschema.xml
@GSETTINGS_RULES@
gsettings_DATA = $(gsettings_SCHEMAS)


EXTRA_DIST = \
    $(icon_DATA)         \
    $(NULL)

CLEANFILES = \
    $(DESKTOP_FILES)        \
    $(appdata_DATA)         \
    $(NULL)

DISTCLEANFILES = \
    $(DESKTOP_FILES)        \
    $(NULL)


###############################################################################

install-icons:
	for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

uninstall-icons:
	-for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		rm -f $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

clean-local:
	rm -f *~

install-data-local: install-icons
uninstall-local: uninstall-icons
