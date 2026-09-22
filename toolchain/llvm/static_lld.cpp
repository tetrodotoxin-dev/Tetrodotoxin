// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "lld/Common/Driver.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)

auto main() -> int {
  const char* arguments[] = {"ld.lld", "--version"};
  const lld::DriverDef drivers[] = {
    {lld::Gnu, &lld::elf::link},
    {lld::WinLink, &lld::coff::link},
  };
  llvm::raw_null_ostream output;
  lld::Result result = lld::lldMain(arguments, output, output, drivers);
  return result.retCode;
}
