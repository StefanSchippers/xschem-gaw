
if CCISGCC
AM_CFLAGS = -Wall
endif

noinst_LIBRARIES = libload.a
libload_a_SOURCES = spicestream.c ss_cazm.c ss_nsout.c ss_spice3.c \
        ss_hspice.c ss_spice2.c ss_wav.c fileformat.c util.c \
	fdbuf.c dbuf.c msglog.c appmem.c appclass.c dataset.c \
	wavevar.c wavetable.c sndstream.c  sndparams.c sockcon.c fdsel.c \
	fdbuf.h dbuf.h msglog.h appmem.h appclass.h dataset.h \
	wavevar.h wavetable.h sndstream.h  sndparams.h ss_wav.h \
	spicestream.h fileformat.h util.h sockcon.h fdsel.h bswap.h \
	stutil.c stutil.h fsutil.c fsutil.h strcatdup.c strcatdup.h \
	incvar.c incvar.h strobj.c strobj.h duprintf.c duprintf.h \
	dlist.c dlist.h dliststr.c dliststr.h array.c array.h \
        strmem.c strmem.h arraystruct.c arraystruct.h \
        fileutil.c fileutil.h arraystr.c arraystr.h \
        datestrconv.c datestrconv.h
	
AM_CPPFLAGS = -I../src @GAW_CFLAGS@

OSTLYCLEANFILES = *.log core FILE *~ prototype.bak
CLEANFILES = *.log core FILE *~ prototype.bak
DISTCLEANFILES = *.log core FILE *~ prototype.bak
MAINTAINERCLEANFILES = *.log core FILE *~ prototype.bak prototype.h \
                        Makefile.in configure 

EXTRA_DIST = stest.c stest.mak
