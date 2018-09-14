include $(TOPDIR)/config.mk

LIB	= librtos_$(CPU).a

START	=
OBJS	=
SOBJS	=

OBJS	+= cpuport.o
OBJS	+= interrupt.o
OBJS	+= vector.o
SOBJS	+= context_gcc.o

all:	.depend $(START) $(LIB)

$(LIB):	$(OBJS) $(SOBJS)
	$(AR) crv $@ $(OBJS) $(SOBJS)

#########################################################################

.depend:	Makefile $(START:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(START:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
