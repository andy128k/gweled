AC_INIT([gweled],
        [0.9.1],
        [https://bugs.launchpad.net/gweled/+filebug],
        [gweled])
        
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
# Use AM_SILENT_RULES if present
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "\${ACLOCAL_FLAGS}")

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

CFLAGS="${CFLAGS} -Wall"

# Check for header files
# ======================
AC_CHECK_HEADERS([fcntl.h], [],
                 [AC_MSG_ERROR([cannot find required header file])])
AC_HEADER_SYS_WAIT

dnl Initialize intltool
GETTEXT_PACKAGE=gweled
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
 
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

#### Check for GTK+ >= 2.14 ####
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.20,,
	AC_MSG_ERROR([GTK+-2.14 is required to compile gweled]))

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)


AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources, glib-compile-resources)

GLIB_GSETTINGS

#### Check for clutter ####
PKG_CHECK_MODULES(CLUTTER, clutter-1.0,,
	AC_MSG_ERROR([clutter-1.0 is required to compile gweled]))

AC_SUBST(CLUTTER_CFLAGS)
AC_SUBST(CLUTTER_LIBS)

#### Check for clutter-gtk ####
PKG_CHECK_MODULES(CLUTTER_GTK, clutter-gtk-1.0,,
	AC_MSG_ERROR([clutter-gtk-1.0 is required to compile gweled]))

AC_SUBST(CLUTTER_GTK_CFLAGS)
AC_SUBST(CLUTTER_GTK_LIBS)

#### Check for libcanberra ####
PKG_CHECK_MODULES(CANBERRA, libcanberra,,
	AC_MSG_ERROR([libcanberra is required to compile gweled]))
	
AC_SUBST(CANBERRA_CFLAGS)
AC_SUBST(CANBERRA_LIBS)

### Check for libcanberra-gtk.h header ###
AC_CHECK_HEADER([canberra.h], [],
                  AC_MSG_ERROR([*** Cannot find canberra-gtk.h header]))
AC_CHECK_LIB(canberra-gtk3, ca_gtk_proplist_set_for_widget, [],
                 AC_MSG_ERROR([*** Cannot find canberra_gtk library]))
AC_SUBST(CANBERRA_GTK_CFLAGS)
AC_SUBST(CANBERRA_GTK_LIBS)

#### Check for libgnome-games-support-1 ####
PKG_CHECK_MODULES(GNOME_GAMES, libgnome-games-support-1,,
	AC_MSG_ERROR([libgnome-games-support-1 is required to compile gweled]))

AC_SUBST(GNOME_GAMES_CFLAGS)
AC_SUBST(GNOME_GAMES_LIBS)

LIBS="$LIBS -export-dynamic"

AC_ARG_ENABLE([setgid],
  [AS_HELP_STRING([--disable-setgid],
    [Disable the use of setgid binaries])],
  [case "${enableval}" in
    yes) setgid=true ;;
    no)  setgid=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --disable-setgid]) ;;
   esac],[setgid=true;enable_setgid=yes])

scoredir='${localstatedir}/games'
scores_group=games
scores_user=games

if test "$enable_setgid" = "yes"; then
  AC_DEFINE([ENABLE_SETGID],[1],[Define if use of setgid binaries is enabled])

  AC_ARG_WITH(scores-group,
    AC_HELP_STRING([--with-scores-group=group],
      [Group for the high score tables and binaries]),
    scores_group="$withval",scores_group="games")
  AC_ARG_WITH(scores-user,
    AC_HELP_STRING([--with-scores-user=user],
      [User for the high score tables]),
    scores_user="$withval",scores_user="games")
fi

AM_CONDITIONAL([ENABLE_SETGID],[test "$enable_setgid" = "yes"])

AC_SUBST(setgid)
AC_SUBST(scores_group)
AC_SUBST(scores_user)
AC_SUBST(scoredir)

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
data/Makefile
])

AC_OUTPUT

echo "
Configuration:

    Source code location:   ${srcdir}
    Compiler:               ${CC}

    Use setgid binaries:    $enable_setgid
    Scores user:            $scores_user
    Scores & setgid group:  $scores_group
"

if grep "$scores_group:" /etc/group > /dev/null; then
:;
else
echo
echo "Warning: The $scores_group group does not exist. Installation will"
echo "         succeed, but high score logging will not work. To specify a"
echo "         different group use the --with-scores-group option."
echo
fi

