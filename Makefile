
CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -ggdb3
LDFLAGS=-lpng $(shell sdl2-config --cflags --libs)

SRC=main.c util.c image.c drawing.c
OBJ=$(addprefix obj/, $(addsuffix .o, $(SRC)))

BIN=hl-png

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

#-------------------------------------------------------------------------------

.PHONY: all clean install

all: $(BIN)

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

install: $(BIN)
	install -D -m 755 $^ -t $(DESTDIR)$(BINDIR)

#-------------------------------------------------------------------------------

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
