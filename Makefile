CC= clang

CFLAGS?=	-Os -g
CFLAGS+=	-Waggregate-return -Wbad-function-cast -Wcast-align
CFLAGS+=	-Wcast-qual -Wdisabled-optimization -Wendif-labels -Wfloat-equal
CFLAGS+=	-Winline -Wmissing-declarations -Wmissing-prototypes -Wnested-externs
CFLAGS+=	-Wpointer-arith -Wredundant-decls -Wsign-compare -Wstrict-prototypes
CFLAGS+=	-Wundef -Wwrite-strings -Werror

CFLAGS+=	-I/opt/X11/include
LDFLAGS+=	-L/opt/X11/lib
LDADD+=		-lX11 -lXext

PROG=   classic-wm
SRCS=   main.c decorations.c pool.c
OBJS=   $(patsubst %.c, %.o, $(SRCS))

.PHONY: clean

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDADD)

clean:
	rm -f $(PROG) $(OBJS)
