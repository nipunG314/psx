export
.DEFAULT_GOAL:= psx

INCLUDE=include
SRC=src
OBJ=$(SRC)/obj
TESTS=tests

CC=gcc
CFLAGS=-I$(INCLUDE) -Wall -pedantic -g
LDFLAGS=-lSDL2
OBJ_CFLAGS=$(CFLAGS) -MMD

SOURCES:=$(shell find $(SRC) -name '*.c')

_OBJECTS=$(SOURCES:.c=.o)
OBJECTS=$(patsubst $(SRC)%,$(OBJ)%,$(_OBJECTS))

DEPS=$(OBJECTS:.o=.d)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(OBJ_CFLAGS) $(LDFLAGS)

-include $(DEPS)   # include all dep files in the makefile

psx: $(OBJECTS)
	$(CC) -o $(SRC)/$@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: test clean

test: psx
	$(MAKE) -C $(TESTS)

clean:
	rm -f $(OBJ)/* $(SRC)/psx
