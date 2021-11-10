INCLUDE=include
SRC=src
OBJ=$(SRC)/obj

CC=gcc
CFLAGS=-I$(INCLUDE) -MMD -Wall -pedantic -g

SOURCES:=$(shell find $(SRC) -name '*.c')

_OBJECTS=$(SOURCES:.c=.o)
OBJECTS=$(patsubst $(SRC)%,$(OBJ)%,$(_OBJECTS))

DEPS=$(OBJECTS:.o=.d)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

psx: $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

-include $(DEPS)   # include all dep files in the makefile

.PHONY: clean
clean:
	rm -f $(OBJ)/* psx
