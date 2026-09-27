// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/linker/provider.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Linker::Provider::select(
    Core::View::Vector<Provider> providers,
    Core::View::Bytes target,
    const Import& imported) -> Utility::Result<Import, Error> {
  Core::Option<Core::View::Bytes> selected;
  for (Count index = 0; index < providers.get_size(); index++) {
    const Provider& provider = providers.get_data()[index];
    if (provider.get_target() != target ||
        provider.get_kind() != imported.get_kind() ||
        provider.get_abi() != imported.get_abi() ||
        provider.get_symbol() != imported.get_symbol()) {
      continue;
    }
    if (selected) {
      return Error::Ambiguous;
    }
    selected = provider.get_identity();
  }
  return selected ? Utility::Result<Import, Error>(
                        imported.select_provider(*selected))
                  : Utility::Result<Import, Error>(Error::Missing);
}
