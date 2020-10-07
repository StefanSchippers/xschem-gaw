rcdatadir = @GAWRCDIR@

if CCISGCC
AM_CFLAGS = -Wall
endif

AM_CPPFLAGS = -I$(top_srcdir)/lib
bin_PROGRAMS = gaw
gaw_SOURCES = gawmain.c gawapp.c userprefs.c gawzoom.c gawmisc.c \
              gawabout.c gawsnd.c gawio.c gawimg.c datafile.c \
              gawcolor.c gawfile.c gawdnd.c gawda.c gawpanel.c \
	      gawcursor.c gawwave.c gawgrip.c gawlabel.c gawhelp.c \
	      gaw.h userprefs.h gawpixmaps.h datafile.h gawdnd.h \
	      gawda.h gawpanel.h gawcursor.h gawwave.h gawgrip.h \
	      gawlabel.h gawmisc.h gawsnd.h gawtext.c gawtext.h \
              gawdraw.c gawdraw.h gawutil.c gawutil.h gawmenus.c \
              gawmenus.h

INCLUDES = -I$(top_srcdir)/lib @GAW_CFLAGS@
gaw_LDADD = $(top_srcdir)/lib/libload.a \
            -L$(prefix)/lib @GAW_LDFLAGS@  -lXext -lm 

###   $(top_srcdir)/lib/libtracemem.a

EXTRA_DIST = gaw-gtkrc gaw-gtkrc.css
rcdata_DATA = gaw-gtkrc gaw-gtkrc.css

MOSTLYCLEANFILES = *.log core FILE *~ prototype.bak
CLEANFILES = *.log core.* FILE *~ prototype.bak
DISTCLEANFILES = *.log core.* FILE *~ prototype.bak
MAINTAINERCLEANFILES = *.log core.* FILE *~ prototype.bak prototype.h \
                        Makefile.in configure 
