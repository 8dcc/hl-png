
CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -ggdb3
LDFLAGS=-lpng $(shell sdl2-config --cflags --libs)

OBJ_FILES=main.c.o util.c.o image.c.o drawing.c.o
OBJS=$(addprefix obj/, $(OBJ_FILES))

BIN=hl-png

#-------------------------------------------------------------------------------

.PHONY: clean all

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
