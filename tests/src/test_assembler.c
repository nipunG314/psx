#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "test_assembler.h"

char * fgetline(char * s, size_t num, FILE * stream) {
  s[0] = 0;
  char *ret = fgets(s, num, stream);

  if (ret) {
    char *pos = strchr(s, '\n');
    if (pos)
      *pos = 0;
  }

  return ret;
}

size_t count_tokens(char * s) {
  size_t c = 1;

  char *ptr = strchr(s, ' ');
  while (ptr) {
    c++;
    ptr = strchr(ptr+1, ' ');
  }

  return c;
}

void tokenize_line(char **tokens, size_t token_count, char *line) {
  tokens[0] = strtok(line, " ");
  for(size_t index = 1; index < token_count; index++) {
    tokens[index] = strtok(0, " ");
  }
}

void assemble_tokens(char **tokens, size_t token_count, FILE * const fp_output) {
  for(size_t index = 0; index < token_count; index++)
    printf("%s\n", tokens[index]);
}

void assemble_test(char const* input, char const* output) {
  FILE * const fp_input = fopen(input, "r");
  FILE * const fp_output = fopen(output, "wb");
  char line[1000];

  if (!fp_input)
    fatal("AssemblerError: Could not open input file: %s", input);
  if (!fp_output)
    fatal("AssemblerError: Could not open output file: %s", output);

  for (;;) {
    char *ret = fgetline(line, sizeof line, fp_input); 
    if (ret) {
      size_t token_count = count_tokens(line);

      char *tokens[token_count];
      tokenize_line(tokens, token_count, line);

      assemble_tokens(tokens, token_count, fp_output);

      if (feof(fp_input))
        break;
    } else
      break;
  }
}
