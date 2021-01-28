dnl Process this file with autoconf to produce a configure script.

AC_INIT([Gweled], [0.9.1], , gweled, [http://gweled.org])
AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

# Use AM_SILENT_RULES if present
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC


# Check for library functions
# ===========================

AC_CHECK_FUNCS([ftruncate memset select strchr], [],
               [AC_MSG_ERROR([cannot find required function])])
AC_CHECK_FUNCS([fork select]) 
AC_CHECK_FUNCS([alarm getpwuid getenv setenv close hstrerror kill socketpair])

AC_FUNC_FORK
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_HEADER_STDC

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
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.14,,
	AC_MSG_ERROR([GTK+-2.14 is required to compile gweled]))

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

#### Check for librsvg 2.0 ####
PKG_CHECK_MODULES(RSVG, librsvg-2.0,,
	AC_MSG_ERROR([librsvg-2.0 is required to compile gweled]))

AC_SUBST(RSVG_CFLAGS)
AC_SUBST(RSVG_LIBS)

#### Check for libcanberra ####
PKG_CHECK_MODULES(CANBERRA, libcanberra,,
	AC_MSG_ERROR([libcanberra is required to compile gweled]))
	
AC_SUBST(CANBERRA_CFLAGS)
AC_SUBST(CANBERRA_LIBS)

### Check for libcanberra-gtk.h header ###
AC_CHECK_HEADER([canberra.h], [],
                  AC_MSG_ERROR([*** Cannot find canberra-gtk.h header]))
AC_CHECK_LIB(canberra-gtk, ca_gtk_proplist_set_for_widget, [],
                 AC_MSG_ERROR([*** Cannot find canberra_gtk library]))
AC_SUBST(CANBERRA_GTK_CFLAGS)
AC_SUBST(CANBERRA_GTK_LIBS)

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

