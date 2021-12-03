#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "cpu.h"
#include "flag.h"
#include "log.h"

FlagSet flag_set = 0x0;

void set_env(int argc, char **argv) {
  for(size_t i=1; i<argc; i++) {
    if (strcmp(argv[i], "--print-pc") == 0)
      set_flag(PRINT_PC);
    else if (strcmp(argv[i], "--print-ins") == 0)
      set_flag(PRINT_INS);
    else if (strcmp(argv[i], "--quiet") == 0)
      log_set_quiet(1);
    else if (strcmp(argv[i], "--output-log") == 0) {
      log_set_quiet(1);
      set_flag(OUTPUT_LOG);
    }
  }
}

int main(int argc, char **argv) {
  set_env(argc, argv);

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    fatal("SDLError: Couldn't init SDL. %s", SDL_GetError());

  Cpu cpu = init_cpu("SCPH1001.BIN");

  while (1) {
    run_next_ins(&cpu);
  }

  destroy_cpu(&cpu);

  SDL_Quit();

  return 0;
}
