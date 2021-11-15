INCLUDE=include
SRC=src
OBJ=$(SRC)/obj
TEST_INCLUDE=tests/include
TEST=tests/src
TEST_OBJ=$(TEST)/obj

CC=gcc
CFLAGS=-I$(INCLUDE) -Wall -pedantic -g
OBJ_CFLAGS=$(CFLAGS) -MMD
TEST_CFLAGS=-I$(TEST_INCLUDE) -I$(INCLUDE) -Wall -pedantic -g
OBJ_TEST_CFLAGS=$(TEST_CFLAGS) -MMD

SOURCES:=$(shell find $(SRC) -name '*.c')
TEST_SOURCES:=$(shell find $(TEST) -name '*.c')

_OBJECTS=$(SOURCES:.c=.o)
OBJECTS=$(patsubst $(SRC)%,$(OBJ)%,$(_OBJECTS))
_TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_OBJECTS=$(patsubst $(TEST)%,$(TEST_OBJ)%,$(_TEST_OBJECTS))

DEPS=$(OBJECTS:.o=.d)
TEST_DEPS=$(TEST_OBJECTS:.o=.d)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(OBJ_CFLAGS)
$(TEST_OBJ)/%.o: $(TEST)/%.c
	$(CC) -c -o $@ $< $(OBJ_TEST_CFLAGS)

-include $(DEPS)   # include all dep files in the makefile

psx: $(SRC)/../main.c $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS)

psx_test: $(TEST)/../main.c $(TEST_OBJECTS) $(OBJECTS)
	$(CC) -o $@ $^ $(TEST_CFLAGS)

all: psx psx_test

.PHONY: run test clean

run: psx
	./psx

test: psx_test
	./psx_test

clean:
	rm -f $(OBJ)/* $(TEST_OBJ)/* psx psx_test
