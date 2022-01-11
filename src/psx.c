#include "psx.h"

Psx init_psx(char const *bios_file_path) {
  Psx psx;

  psx.cycles_counter = MAKE_Cycles(0);

  return psx;
}
