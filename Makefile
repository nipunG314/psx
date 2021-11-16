export

INCLUDE=include
SRC=src
OBJ=$(SRC)/obj

CC=gcc
CFLAGS=-I$(INCLUDE) -Wall -pedantic -g
OBJ_CFLAGS=$(CFLAGS) -MMD

SOURCES:=$(shell find $(SRC) -name '*.c')

_OBJECTS=$(SOURCES:.c=.o)
OBJECTS=$(patsubst $(SRC)%,$(OBJ)%,$(_OBJECTS))

DEPS=$(OBJECTS:.o=.d)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(OBJ_CFLAGS)

-include $(DEPS)   # include all dep files in the makefile

psx: $(SRC)/../main.c $(OBJECTS)
	$(MAKE) all -C tests
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: run test clean

run: psx
	./psx

test:
	$(MAKE) test -C tests

clean:
	rm -f $(OBJ)/* psx
	$(MAKE) clean -C tests
