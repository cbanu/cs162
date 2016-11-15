all: main wc map

CFLAGS=-g -Wall
LDFLAGS=-g

wc.o: wc.c
	gcc $(CFLAGS) -c wc.c
wc: wc.o
	gcc $(LDFLAGS) wc.o -o wc

main.o: main.c
	gcc $(CFLAGS) -c main.c
main: main.o
	gcc $(LDFLAGS) main.o -o main

map.o: map.c
	gcc $(CFLAGS) -c map.c
map: map.o
	gcc $(LDFLAGS) map.o -o map

clean:
	rm -f wc.o wc main.o main map.o map
