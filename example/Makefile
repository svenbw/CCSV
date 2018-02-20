LIBS = -L/usr/lib
INCL = -I..

CFLAGS = -Werror -Wall
OBJ = ../csv.o example.o

all: example

%.o: %.c
	$(CC) $(INCL) $(CFLAGS) -c -o $@ $<

example: $(OBJ)
	$(CC) $^ $(LIBS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f example *.o ../*.o
