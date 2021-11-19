TEST_INCLUDE=include
TEST_SRC=src
TEST_OBJ=$(TEST_SRC)/obj

TEST_CFLAGS=-I$(TEST_INCLUDE) -I../$(TEST_INCLUDE) -Wall -pedantic -g
TEST_OBJ_CFLAGS=$(TEST_CFLAGS) -MMD

TEST_SOURCES:=$(shell find $(TEST_SRC) -name 'unity.c')
ASM_SOURCES:=$(shell find $(TEST_SRC) -name 'test_assembler.c')

FILTERED_OBJECTS=$(filter-out src/obj/main.o, $(OBJECTS))
_TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_OBJECTS=$(patsubst $(TEST_SRC)%,$(TEST_OBJ)%,$(_TEST_OBJECTS)) $(patsubst %, ../%, $(FILTERED_OBJECTS))
_ASM_OBJECTS=$(ASM_SOURCES:.c=.o)
ASM_OBJECTS=$(patsubst $(TEST_SRC)%,$(TEST_OBJ)%,$(_ASM_OBJECTS)) $(patsubst %, ../%, $(FILTERED_OBJECTS))

$(TEST_OBJ)/%.o: $(TEST_SRC)/%.c
	$(CC) -c -o $@ $< $(TEST_OBJ_CFLAGS)

psx_test: $(TEST_SRC)/../main.c $(TEST_OBJECTS)
	$(CC) -o $@ $^ $(TEST_CFLAGS)

test_assembler: $(TEST_SRC)/../test_assembler.c $(ASM_OBJECTS)
	$(CC) -o $@ $^ $(TEST_CFLAGS)

.PHONY: clean tests

clean:
	rm -rf $(TEST_OBJ)/* psx_test asm_tests/*.bin

test: psx_test test_assembler
	./test_assembler
	./psx_test
