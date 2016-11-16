EXECUTABLES=main wc map
CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-g

.SECONDEXPANSION:

all: $(EXECUTABLES)

$(EXECUTABLES): $$@.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLES) $(EXECUTABLES:=.o)
