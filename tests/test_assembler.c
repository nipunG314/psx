#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "log.h"

typedef struct dirent dirent;

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
  uint32_t ins;

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

  fwrite(&ins, sizeof ins, 1, fp_output);
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

int main(void) {
  dirent *dir_entry;
  DIR * const dir = opendir("asm_tests");

  if (dir) {
    while ((dir_entry = readdir(dir))) {
      if (dir_entry->d_type == DT_REG) {
        char *ptr = strstr(dir_entry->d_name, ".asm");

        if (ptr) {
          size_t filename_length = strlen(dir_entry->d_name) + 10;
          char input_file[filename_length + 1];
          char output_file[filename_length + 1];
          strncpy(input_file, "asm_tests/", 11);
          strncpy(input_file + 10, dir_entry->d_name, filename_length - 10);
          strncpy(output_file, "asm_tests/", 11);
          strncpy(output_file + 10, dir_entry->d_name, filename_length - 10);
          input_file[filename_length] = output_file[filename_length] = 0;

          size_t index = filename_length - 3;
          output_file[index++] = 'b';
          output_file[index++] = 'i';
          output_file[index++] = 'n';

          assemble_test(input_file, output_file);
        }
      }
    }
  }

  return 0;
}
