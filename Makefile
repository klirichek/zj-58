CC=gcc
RM=rm -f
CFLAGS=-Wl,-rpath,/usr/lib -Wall -fPIC -O3 -std=gnu89
LDFLAGS=
LDLIBS=-lcupsimage -lcups

SRCS=rastertozj.c
OBJS=$(subst .c,.o,$(SRCS))

all: rastertozj

rastertozj: $(OBJS)
	gcc $(LDFLAGS) -o rastertozj rastertozj.o $(LDLIBS)

rastertozj.o: rastertozj.c
	gcc $(CFLAGS) -c rastertozj.c

clean:
	rm -f rastertozj rastertozj.o
