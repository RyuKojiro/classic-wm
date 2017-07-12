CFLAGS?=  -Os -std=c99 #-g
CFLAGS+=  -Wall -Werror

CFLAGS+=  -I/opt/X11/include
LDFLAGS+= -L/opt/X11/lib
LDLIBS+=  -lX11 -lXext

PROG= classic-wm
SRCS= main.c decorations.c pool.c
OBJS= $(SRCS:.c=.o)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $(OBJS)

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: clean
