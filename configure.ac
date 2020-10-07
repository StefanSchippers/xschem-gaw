dnl **** Process this file with autoconf to produce a configure script.
dnl *** Remember [] is the quoting mechanism of m4

AC_PREREQ(2.61)
AC_INIT([gaw3], [1.0], [www.rvq.fr])
AC_CONFIG_SRCDIR([src/gawmain.c])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER([config.h])

AC_PREFIX_DEFAULT("/usr/local")

AC_DEFINE([PACKAGE], [gaw], [Gtk analog waveform viewer - tool for viewing analog data, such as the output of Spice simulations])
PACKAGE=gaw3
VERSION=20200922
echo Configuring $PACKAGE version $VERSION

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

# Checks for programs.
AC_PROG_CC
AM_CONDITIONAL(CCISGCC, test "$GCC" = "yes")
AC_PROG_RANLIB
AC_PROG_MKDIR_P

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h float.h stdlib.h string.h unistd.h])

AC_SYS_LARGEFILE

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_FUNC_FSEEKO
AC_CHECK_FUNCS([mkdir pow regcomp strcasecmp strchr strcspn strdup strerror strrchr strspn strstr])

############################################################################
# Check for pkg-config

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test $PKG_CONFIG = no; then
   AC_MSG_ERROR([Cannot find pkg-config, make sure it is installed and in your PATH])
fi

############################################################################
# Check for byteswap.h
AC_CHECK_HEADERS([byteswap.h])
AC_CHECK_HEADERS([endian.h])
AC_CHECK_HEADERS([libkern/OSByteOrder.h])
AC_CHECK_HEADERS([sys/endian.h])


############################################################################
dnl ****** INTERNATIONALIZATION **********************

GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
        prefix=${ac_default_prefix}
fi
DATADIRNAME=share
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale", [Location of package locale files])


############################################################################
# Check for alsa libs
# 
AC_CHECK_HEADERS([alsa/pcm.h], [have_pcm="yes"], [have_pcm="no"],
	  [#include <alsa/asoundlib.h>])
AC_CHECK_HEADERS([alsa/mixer.h], [have_mixer="yes"], [have_mixer="no"],
	  [#include <alsa/asoundlib.h>])

AC_ARG_ENABLE(gawsound,
      [  --enable-gawsound=[yes/no]    Turn on or off compilation of alsa libs interface [default=yes]],
      [case "${enableval}" in
        yes) gawsound=true ;;
        no)  gawsound=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-gawsound) ;;
      esac],[gawsound=true])

if test "$have_pcm" = "yes" -a  "$have_mixer" = "yes" ; then
  if test "$gawsound" = "true" ; then
	AM_PATH_ALSA(1.0.16)
	dnl Check components
	ALSA_VERSION=`$PKG_CONFIG alsa --modversion`
  else
        echo "Alsa libs interface compilation is disabled."
  fi
else
  if test "$gawsound" = "true" ; then
        echo "Alsa libs interface compilation is disabled."
  fi
fi

############################################################################
# Check for gtk+ 3.0 start
# 

PKG_CHECK_MODULES(GTK300, gtk+-3.0 >= 3.0.0, GTK300="yes", no_GTK300="yes")

# This next bit of code figures out what gtk we need to use.
if test "$GTK300" = "yes"
then

   AC_DEFINE(HAVE_GTK300, 1, [If gtk+ 3.00.x has been installed, define this])
   GTK_CFLAGS=$GTK300_CFLAGS
   GTK_LIBS=$GTK300_LIBS
   GTK_VERSION=`$PKG_CONFIG gtk+-3.0 --modversion`

   # Search for glib
   PKG_CHECK_MODULES(GLIB228, glib-2.0 >= 2.28.0, GLIB228="yes", no_GLIB228="yes")
   if test "$GLIB228" != "yes"
   then
       AC_MSG_ERROR([Cannot find glib 2.28.x, install it and rerun ./configure.])
   fi
   AC_DEFINE(HAVE_GLIB, 1, [If glib 2.28.x has been installed, define this])
   GLIB_CFLAGS=$GLIB228_CFLAGS
   GLIB_LIBS=$GLIB228_LIBS
   GLIB_VERSION=`$PKG_CONFIG glib-2.0 --modversion`

else
   GTK_VERSION=""
fi

if test "$GTK_VERSION" = ""
then
   AC_MSG_ERROR([Cannot find gtk+ 3.x or later, please install gtk+.])
fi

# 
# Check for gtk+ 3.0 end
############################################################################


# Finally create the final CFLAGS and LDFLAGS for use in the makefiles
GAW_CFLAGS="$GTK_CFLAGS"
GAW_LDFLAGS="$GTK_LIBS"

# Expand the prefix variable
if eval "test x$prefix = xNONE"; then
 dprefix=$ac_default_prefix
else
 dprefix=$prefix
fi

DATADIR=gaw
gawtopdir=$dprefix/share/$DATADIR
GAWRCDIR=$gawtopdir

# this has to be expanded ( no ${prefix} ) --
AC_DEFINE_UNQUOTED(GAWDATADIR, "$gawtopdir", [gaw data directory])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Currently running version of gaw])
AC_DEFINE_UNQUOTED(GAWRCDIR, "$gawtopdir", [gaw's rc directory])


# Makefile.in variable substitution
AC_SUBST(GAW_CFLAGS)
AC_SUBST(GAW_LDFLAGS)
AC_SUBST(GAWRCDIR)

# Create all the necessary derived files
AC_CONFIG_FILES([Makefile src/Makefile lib/Makefile po/Makefile.in])

AC_OUTPUT

expandedGAWDATADIR=`eval "echo $gawtopdir"`
expandedGAWRCDIR=`eval "echo $gawtopdir"`
expandedGAWLOCALE=`eval "echo ${prefix}/${DATADIRNAME}/locale"`

AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

   GLIB library version:             $GLIB_VERSION
   GTK+ library version:             $GTK_VERSION
   Alsa version:                     $ALSA_VERSION
   data directory:                   $expandedGAWDATADIR
   locale directory:                 $expandedGAWLOCALE
])
