#include <stdio.h>
#include <stdint.h>
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

uint32_t convert_le(uint32_t hex) {
  uint32_t b0 = hex & 0xFF;
  uint32_t b1 = (hex & 0xFF00) >> 8;
  uint32_t b2 = (hex & 0xFF0000) >> 16;
  uint32_t b3 = (hex & 0xFF000000) >> 24;

  return b3 | (b2 << 8) | (b1 << 16) | (b0 << 24);
}

void assemble_tokens(char **tokens, size_t token_count, FILE * const fp_output) {
  uint32_t ins;
  uint32_t ins_le;

  if (strcmp(tokens[0], "j") == 0) {
    uint32_t imm_jump = (strtol(tokens[1], 0, 0) >> 2) & 0x03FFFFFF;
    ins = imm_jump | 0x08000000;
  } else if (strcmp(tokens[0], "lui") == 0) {
    uint32_t rt = strtol(tokens[1], 0, 0) << 16; 
    uint32_t imm = strtol(tokens[2], 0, 0);
    ins = rt | imm | 0x3C000000;
  } else {
    fatal("AssemblerError: Could not match test instruction. Instruction Token: %s", tokens[0]);
  }

  ins_le = convert_le(ins);
  fwrite(&ins_le, sizeof ins_le, 1, fp_output);
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

  fclose(fp_input);
  fclose(fp_output);
}
