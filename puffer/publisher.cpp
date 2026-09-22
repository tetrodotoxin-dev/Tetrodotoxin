// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/publisher.hpp"

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <ftw.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/source/reference.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

static auto join(Core::View::Bytes root, Core::View::Bytes relative)
    -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes result(root);
  if (!result.is_empty() && result[result.get_size() - 1] != '/') {
    result.append('/');
  }
  result.concat(relative);
  return result;
}

static auto terminate(Core::View::Bytes path) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes result(path);
  result.append(0);
  return result;
}

static auto is_relative_product(Core::View::Bytes name) -> Bool {
  BAIL_IF(name.is_empty() || name[0] == '/');
  Count start = 0;
  for (Count index = 0; index <= name.get_size(); index++) {
    if (index != name.get_size() && name[index] != '/') {
      continue;
    }
    Core::View::Bytes segment = name.slice(start, index - start);
    BAIL_IF(segment.is_empty() || segment == "."_view || segment == ".."_view);
    start = index + 1;
  }
  return True;
}

static auto coordinate(Core::View::Bytes name)
    -> Core::Option<Core::View::Bytes> {
  Count separators = 0;
  for (Count index = 0; index < name.get_size(); index++) {
    if (name[index] != '/') {
      continue;
    }
    separators++;
    if (separators == 2) {
      BAIL_IF(index + 1 == name.get_size());
      return name.slice(0, index);
    }
  }
  return {};
}

static auto create_directories(Core::View::Bytes path) -> Bool {
  for (Count index = 1; index < path.get_size(); index++) {
    if (path[index] != '/') {
      continue;
    }
    Core::View::Bytes directory = path.slice(0, index);
    if (directory.is_empty()) {
      continue;
    }
    Memory::Dynamic::Bytes terminated = terminate(directory);
    if (mkdir(
            reinterpret_cast<const char*>(terminated.get_view().get_data()),
            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 &&
        errno != EEXIST) {
      return False;
    }
  }
  return True;
}

static auto remove_entry(const char* path, const struct stat*, S32, struct FTW*)
    -> S32 {
  return remove(path);
}

static auto remove_tree(Core::View::Bytes path) -> void {
  Memory::Dynamic::Bytes terminated = terminate(path);
  nftw(
      reinterpret_cast<const char*>(terminated.get_view().get_data()),
      remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}

static auto path_kind(Core::View::Bytes path) -> Core::Option<Bool> {
  Memory::Dynamic::Bytes terminated = terminate(path);
  struct stat status = {};
  if (lstat(
          reinterpret_cast<const char*>(terminated.get_view().get_data()),
          &status) == 0) {
    return Bool(S_ISDIR(status.st_mode));
  }
  return errno == ENOENT ? Core::Option<Bool>() : Core::Option<Bool>(False);
}

static auto commit(Core::View::Bytes stage, Core::View::Bytes target) -> Bool {
  auto existing = path_kind(target);
  BAIL_IF(existing && !*existing);
  Memory::Dynamic::Bytes stage_name = terminate(stage);
  Memory::Dynamic::Bytes target_name = terminate(target);
  if (!existing) {
    return rename(
               reinterpret_cast<const char*>(stage_name.get_view().get_data()),
               reinterpret_cast<const char*>(
                   target_name.get_view().get_data())) == 0;
  }

  S64 exchanged = syscall(
      SYS_renameat2, AT_FDCWD,
      reinterpret_cast<const char*>(stage_name.get_view().get_data()), AT_FDCWD,
      reinterpret_cast<const char*>(target_name.get_view().get_data()),
      RENAME_EXCHANGE);
  BAIL_IF(exchanged != 0);
  remove_tree(stage);
  return True;
}

auto Puffer::Publisher::publish(const Tetrodotoxin::Source::Pack& products) const
    -> Bool {
  const Tetrodotoxin::Source::Layout& layout = products.get_layout();
  BAIL_IF(layout.is_empty());

  Core::Option<Core::View::Bytes> product_coordinate;
  Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Language::Product>>
      retained;
  for (Count index = 0; index < layout.get_size(); index++) {
    auto abstract = layout.get_abstract(index);
    auto product = abstract
                       ? abstract->select<Tetrodotoxin::Language::Product>()
                       : Core::Option<const Tetrodotoxin::Language::Product&>();
    BAIL_IF(!product || !is_relative_product(product->get_name()));
    Memory::Allocator::Arena arena;
    auto normalized = System::Path::normalize(arena, product->get_name());
    auto selected_coordinate = coordinate(product->get_name());
    BAIL_IF(
        !normalized || *normalized != product->get_name() ||
        !selected_coordinate ||
        (product_coordinate && *product_coordinate != *selected_coordinate));
    product_coordinate = *selected_coordinate;
    for (const auto& prior : retained.get_view()) {
      BAIL_IF(prior.get().get_name() == product->get_name());
    }
    retained.emplace(*product);
  }

  Memory::Dynamic::Bytes target = join(root, *product_coordinate);
  Count slash = product_coordinate->get_size();
  while (slash != 0 && (*product_coordinate)[slash - 1] != '/') {
    slash--;
  }
  BAIL_IF(slash == 0 || !create_directories(target.get_view()));
  Core::View::Bytes identity = product_coordinate->slice(0, slash - 1);
  Core::View::Bytes version = product_coordinate->slice(slash);
  Memory::Dynamic::Bytes stage_relative(identity);
  stage_relative.concat("/."_view);
  stage_relative.concat(version);
  Serialization::Stream::Textual<Memory::Dynamic::Bytes> stage_name(
      stage_relative);
  stage_name << ".puffer."_view << S64(getpid()) << ".tmp"_view;
  Memory::Dynamic::Bytes stage = join(root, stage_relative.get_view());
  Memory::Dynamic::Bytes terminated_stage = terminate(stage.get_view());
  if (mkdir(
          reinterpret_cast<const char*>(terminated_stage.get_view().get_data()),
          S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
    return False;
  }

  for (const auto& selected : retained.get_view()) {
    const Tetrodotoxin::Language::Product& product = selected.get();
    Core::View::Bytes relative =
        product.get_name().slice(product_coordinate->get_size() + 1);
    Memory::Dynamic::Bytes destination = join(stage.get_view(), relative);
    if (!create_directories(destination.get_view()) ||
        !System::File::write(product.get_value(), destination.get_view())) {
      remove_tree(stage.get_view());
      return False;
    }
  }

  if (!commit(stage.get_view(), target.get_view())) {
    remove_tree(stage.get_view());
    return False;
  }
  return True;
}
