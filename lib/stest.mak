CC = gcc `pkg-config --cflags gtk+-2.0`
CCLD = gcc `pkg-config --libs  gtk+-2.0`

COMMON = /home/hq/prj/common

CPPFLAGS = -DHAVE_CONFIG_H
LDFLAGS = 
LIBS = -lasound -lm -ldl -lpthread 
INCLUDES = -I. -I.. -DMSG_DEBUG
CFLAGS = -g -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CCLD) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@
LFS    = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE


SRCS = stest.c spicestream.c fileformat.c fdbuf.c dbuf.c msglog.c appmem.c \
       appclass.c ss_cazm.c dataset.c wavevar.c wavetable.c util.c \
       sndstream.c  sndparams.c ss_hspice.c ss_nsout.c ss_spice2.c \
       ss_spice3.c ss_wav.c strmem.c duprintf.c	

SOBJS = $(SRCS:.c=.o)
t_OBJECTS = $(SOBJS)
t_LDFLAGS = 

.c.o:
	$(COMPILE) -c $<


stest : $(t_OBJECTS)
	$(LINK) $(t_LDFLAGS) $(t_OBJECTS) $(t_LDADD)

.link :
	ln -sf $(COMMON)/msglog.c
	ln -sf $(COMMON)/msglog.h
	ln -sf $(COMMON)/appmem.c
	ln -sf $(COMMON)/appmem.h
	ln -sf $(COMMON)/appclass.c
	ln -sf $(COMMON)/appclass.h
	ln -sf $(COMMON)/dbuf.c
	ln -sf $(COMMON)/dbuf.h
	ln -sf $(COMMON)/fdbuf.c
	ln -sf $(COMMON)/fdbuf.h
	ln -sf $(COMMON)/sockcon.c
	ln -sf $(COMMON)/sockcon.h
	ln -sf $(COMMON)/strmem.c
	ln -sf $(COMMON)/strmem.h

rmlink:
	rm -f msglog.c msglog.h appmem.c appmem.h \
	     appclass.c appclass.h dbuf.c dbuf.h fdbuf.c fdbuf.h \
	     sockcon.c sockcon.h

clean :
	rm -f *.o *~ stest
