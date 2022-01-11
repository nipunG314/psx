#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "psx.h"
#include "log.h"

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    fatal("SDLError: Couldn't init SDL. %s", SDL_GetError());

  Psx psx = init_psx("SCPH1001.BIN");

/*  Cpu cpu = init_cpu("SCPH1001.BIN");

  while (1) {
    run_next_ins(&cpu);
  }

  destroy_cpu(&cpu);*/

  SDL_Quit();

  return 0;
}
