#ifndef EXCEPTION_H
#define EXCEPTION_H

typedef enum Exception {
  SysCall = 0x8,
  Overflow = 0xc,
  LoadAddressError = 0x4,
  StoreAddressError = 0x5,
  Break = 0x9,
  CoprocessorError = 0xb
} Exception;

#endif
