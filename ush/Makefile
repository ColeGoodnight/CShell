CC     = gcc
CFLAGS = -g -Wall -I.
DEPS   = defn.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ush: ush.o expand.o builtin.o strmode.o
	$(CC) -o ush ush.o expand.o builtin.o strmode.o

.PHONY = clean

clean:
	rm -f *.o 
	rm -f ush