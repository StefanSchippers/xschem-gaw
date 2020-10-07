CC = gcc
CCLD = gcc

COMMON = /home/hq/prj/common

CPPFLAGS = 
LDFLAGS = 
LIBS = 
INCLUDES = -I. -DLINUX -DMSG_DEBUG
CFLAGS = -g -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CCLD) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@
LFS    = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE


#********************* Files ******************************
COMSRCS = dbuf.c sockcon.c fdsel.c msglog.c appmem.c appclass.c strmem.c duprintf.c

SRCS = main.c $(COMSRCS) $(LOCALSRCS)

COMHEADERS = appclass.h  appmem.h  dbuf.h  msglog.h  sockcon.h fdsel.h  strmem.h \
             duprintf.h
LOCALHEADERS = 
HEADERS =

COMLINKS = $(COMSRCS) $(COMHEADERS)
LOCALLINKS = $(LOCALSRCS) $(LOCALHEADERS)

#********************* end Files ***************************

SOBJS = $(SRCS:.c=.o)
t_OBJECTS = $(SOBJS)
t_LDFLAGS = 

.c.o:
	$(COMPILE) -c $<


ttg: $(t_OBJECTS)
	$(LINK) $(t_LDFLAGS) $(t_OBJECTS) $(t_LDADD)

links :
	make rmlink
	make .link

.link :
	# echo $(COMLINKS) 
	for file in $(COMLINKS); do \
	  ln -sf $(COMMON)/$$file; \
	done
	# echo $(LOCALLINKS) 
	for file in $(LOCALLINKS); do \
	 ln -sf $(COMTS)/$$file; \
	 done
	touch .link

rmlink:
	rm -f .link $(COMLINKS)  $(LOCALLINKS)


clean :
	rm -f *.o *~ ttg
