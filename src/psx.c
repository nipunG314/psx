#include "psx.h"

Psx init_psx(char const *bios_file_path) {
  Psx psx;

  psx.cycles_counter = MAKE_Cycles(0);
  psx.bios = init_bios(bios_file_path); 
  psx.ram = init_ram();

  return psx;
}
