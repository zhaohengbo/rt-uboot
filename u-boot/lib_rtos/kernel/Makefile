include $(TOPDIR)/config.mk

LIB	= librtos_kernel.a

START	=
OBJS	=
SOBJS	=

OBJS	+= clock.o
OBJS	+= components.o
OBJS	+= device.o
OBJS	+= idle.o
OBJS	+= ipc.o
OBJS	+= irq.o
OBJS	+= kservice.o
OBJS	+= mem.o
OBJS	+= memheap.o
OBJS	+= mempool.o
OBJS	+= object.o
OBJS	+= scheduler.o
OBJS	+= signal.o
OBJS	+= slab.o
OBJS	+= thread.o
OBJS	+= timer.o

all:	.depend $(START) $(LIB)

$(LIB):	$(OBJS) $(SOBJS)
	$(AR) crv $@ $(OBJS) $(SOBJS)

#########################################################################

.depend:	Makefile $(START:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(START:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
