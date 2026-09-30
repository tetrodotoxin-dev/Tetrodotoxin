// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/dialects/build/input.hpp"
#include "tetrodotoxin/dialects/build/workspace.hpp"
#include "tetrodotoxin/dialects/source/input.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/export.hpp"
#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/negotiation/library.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;

static auto view(perimortem_view_bytes bytes) -> View::Bytes {
  return {bytes.data, bytes.size};
}

static auto valid_name(perimortem_view_bytes bytes) -> bool {
  if (!bytes.data || !bytes.size) {
    return false;
  }
  for (Count i = 0; i < bytes.size; ++i) {
    if (!bytes.data[i]) {
      return false;
    }
  }
  return true;
}

static auto reject(
    const tetrodotoxin_build_input& input,
    View::Bytes message,
    View::Bytes subject = {}) -> bool {
  if (input.report) {
    Dynamic::Bytes text(message);
    if (!subject.is_empty()) {
      text.concat(": "_view);
      text.concat(subject);
    }
    input.report(input.reporter, {text.get_view().get_data(), text.get_size()});
  }
  return false;
}

class BuildModule {
 public:
  BuildModule(View::Bytes name, Library library)
      : name(name), library(Data::take(library)) {}
  Dynamic::Bytes name;
  Library library;
};

// A member owns exactly the reservation acquired from its real provider. Moving
// the inventory transfers that obligation, while navigation only lends a view.
class BuildMember {
 public:
  BuildMember(View::Bytes name, Policies::Borrowed root)
      : name(name), root(root) {}
  BuildMember(const BuildMember&) = delete;
  BuildMember(BuildMember&& other)
      : name(Data::take(other.name)), root(other.root), owns(other.owns) {
    other.owns = false;
  }
  ~BuildMember() {
    if (owns) {
      root.release();
    }
  }
  Dynamic::Bytes name;
  Policies::Borrowed root;

 private:
  bool owns = true;
};

class BuildWorkspace {
 public:
  BuildWorkspace(Object<> storage, View::Bytes output)
      : storage(storage), output(output) {}
  static auto create(View::Bytes output) -> BuildWorkspace& {
    static constexpr Object<>::Descriptor descriptor{
      sizeof(BuildWorkspace), alignof(BuildWorkspace), [](U8* payload) {
        reinterpret_cast<BuildWorkspace*>(payload)->~BuildWorkspace();
      }};
    const auto storage = Object<>::create(descriptor);
    return *new (storage.get_payload(), Placement::Construct)
        BuildWorkspace(storage, output);
  }
  auto release() const -> void { storage.release(); }
  auto borrowed() const -> Policies::Borrowed {
    static const ttx_borrowed_ops operations{
      workspace<true>().get_abi().operations->abstract, [](const void* source) {
        static_cast<const BuildWorkspace*>(source)->release();
      }};
    return Policies::Borrowed({this, &operations});
  }

  template <bool Acquired>
  auto workspace() const -> Build::Workspace {
    static const tetrodotoxin_build_workspace_ops operations{
      {
        [](const void*, perimortem_uuid id) -> ttx_binding_status {
          const Perimortem::System::Uuid contract(id);
          if (contract == Abstract::contract_id ||
              contract == Build::Workspace::contract_id ||
              contract == Capabilities::Borrow::contract_id) {
            return TTX_BINDING_SATISFIED;
          }
          if (contract == Policies::Borrowed::contract_id) {
            return Acquired ? TTX_BINDING_SATISFIED : TTX_BINDING_REJECTED;
          }
          return TTX_BINDING_UNKNOWN;
        },
        [](const void* source, perimortem_uuid id,
           ttx_storage requested) -> ttx_binding_status {
          const Perimortem::System::Uuid contract(id);
          const Storage target(requested);
          const auto& owner = *static_cast<const BuildWorkspace*>(source);
          if (contract == Abstract::contract_id) {
            return static_cast<ttx_binding_status>(Binding::provide<Abstract>(
                {source, &operations.abstract}, target));
          }
          if (contract == Build::Workspace::contract_id) {
            return static_cast<ttx_binding_status>(
                Binding::provide<Build::Workspace>(
                    {source, &operations}, target));
          }
          if (contract == Capabilities::Borrow::contract_id) {
            static const ttx_borrow_ops borrow_operations{
              operations.abstract,
              [](const void* source,
                 ttx_borrowed* output) -> ttx_binding_status {
                const auto& owner = *static_cast<const BuildWorkspace*>(source);
                owner.storage.retain();
                *output = owner.borrowed().get_abi();
                return TTX_BINDING_SATISFIED;
              }};
            return static_cast<ttx_binding_status>(
                Binding::provide<Capabilities::Borrow>(
                    {source, &borrow_operations}, target));
          }
          if (contract == Policies::Borrowed::contract_id) {
            if constexpr (Acquired) {
              return static_cast<ttx_binding_status>(
                  Binding::provide<Policies::Borrowed>(
                      owner.borrowed().get_abi(), target));
            }
            return TTX_BINDING_REJECTED;
          }
          return TTX_BINDING_UNKNOWN;
        },
        [](const void*) -> perimortem_view_bytes { return {}; },
        [](const void* source) -> ttx_abstract {
          return {source, &operations.abstract};
        },
        [](const void* source, perimortem_view_bytes route) -> ttx_abstract {
          const auto members =
              static_cast<const BuildWorkspace*>(source)->members.get_view();
          for (Count i = 0; i < members.get_size(); ++i) {
            const auto& member = members.get_data()[i];
            if (member.name == view(route)) {
              return member.root.Abstract::get_abi();
            }
          }
          return ttx_unknown();
        },
        [](const void* source, ttx_concept_visitor visitor) {
          const auto members =
              static_cast<const BuildWorkspace*>(source)->members.get_view();
          for (Count i = 0; i < members.get_size(); ++i) {
            const auto& member = members.get_data()[i];
            const auto name = member.name.get_view();
            visitor.receive(
                visitor.source, {name.get_data(), name.get_size()},
                member.root.Abstract::get_abi());
          }
        },
      },
      [](const void* source) -> perimortem_view_bytes {
        const auto output =
            static_cast<const BuildWorkspace*>(source)->output.get_view();
        return {output.get_data(), output.get_size()};
      }};
    return Build::Workspace({this, &operations});
  }

  auto build(const tetrodotoxin_build_input& input) -> bool {
    if (!valid_name(input.output) || !input.module_count ||
        !input.source_count || !input.modules || !input.sources) {
      return reject(
          input,
          "Build requires modules, sources and an output directory"_view);
    }

    // Resolve the complete routing description before loading providers. Source
    // selection is explicit because several dialects may accept the same native
    // input representation without assigning the same meaning to its bytes.
    for (Count i = 0; i < input.module_count; ++i) {
      const auto& module = input.modules[i];
      if (!valid_name(module.name) || !valid_name(module.path)) {
        return reject(input, "Invalid module declaration"_view);
      }
      for (Count j = 0; j < i; ++j) {
        if (view(module.name) == view(input.modules[j].name)) {
          return reject(input, "Duplicate module name"_view, view(module.name));
        }
      }
    }
    for (Count i = 0; i < input.source_count; ++i) {
      const auto& source = input.sources[i];
      if (!valid_name(source.name) || !valid_name(source.path) ||
          !valid_name(source.importer)) {
        return reject(input, "Invalid source declaration"_view);
      }
      for (Count j = 0; j < i; ++j) {
        if (view(source.name) == view(input.sources[j].name)) {
          return reject(input, "Duplicate source name"_view, view(source.name));
        }
      }
      bool found = false;
      for (Count j = 0; j < input.module_count; ++j) {
        found |= bool(view(source.importer) == view(input.modules[j].name));
      }
      if (!found) {
        return reject(input, "Unknown importer"_view, view(source.importer));
      }
    }

    Perimortem::Memory::Allocator::Arena errors;
    for (Count i = 0; i < input.module_count; ++i) {
      const auto& declared = input.modules[i];
      const bool loaded =
          Library::open(view(declared.path), errors)
              .visit(
                  [&](Library library) {
                    modules.emplace(
                        BuildModule(view(declared.name), Data::take(library)));
                    return true;
                  },
                  [&](View::Bytes error) {
                    return reject(input, error, view(declared.name));
                  });
      if (!loaded) {
        return false;
      }
    }

    for (Count i = 0; i < input.source_count; ++i) {
      const auto& declared = input.sources[i];
      auto bytes = Perimortem::System::File::read(view(declared.path));
      if (!bytes) {
        return reject(input, "Unable to read source"_view, view(declared.path));
      }
      struct SourceBytes {
        View::Bytes bytes;
        auto get_data() const -> View::Bytes { return bytes; }
      } source_bytes{bytes->get_view()};
      const tetrodotoxin_source_input source{
        declared.path, Abstract::provide(source_bytes).get_abi()};
      bool retained = false;
      auto receive = [&](Query query) {
        return query.bind<Capabilities::Import>().visit(
            [&](Capabilities::Import importer) {
              auto observe = [&](Abstract root) {
                root.bind<Capabilities::Borrow>().visit(
                    [&](Capabilities::Borrow borrow) {
                      borrow.borrow().visit(
                          [&](Policies::Borrowed answer) {
                            members.emplace(
                                BuildMember(view(declared.name), answer));
                            retained = true;
                          },
                          [](Binding::Failure) {});
                    },
                    [](Binding::Failure) {});
              };
              return importer.visit(
                  &source,
                  Compiled<Native<tetrodotoxin_source_input>::reference>::
                      get_representation(),
                  observe);
            },
            [](Binding::Failure failure) {
              return static_cast<Binding::Status>(failure);
            });
      };
      Binding::Status status = Binding::Status::Rejected;
      const auto plugins = modules.get_view();
      for (Count j = 0; j < plugins.get_size(); ++j) {
        const auto& plugin = plugins.get_data()[j];
        if (plugin.name == view(declared.importer)) {
          status = plugin.library.visit(Query(), Receiver(receive));
          break;
        }
      }
      if (status != Binding::Status::Satisfied || !retained) {
        return reject(
            input, "Importer did not supply a retained source"_view,
            view(declared.name));
      }
    }

    // Import callbacks have all ended before any terminal sees the workspace.
    // Keep library owners rather than cached Query views because a provider may
    // supply those views from invocation local state.
    Count terminal_count = 0;
    const auto plugins = modules.get_view();
    for (Count i = 0; i < plugins.get_size(); ++i) {
      const auto& plugin = plugins.get_data()[i];
      bool terminal = false;
      auto receive = [&](Query query) {
        return query.bind<Capabilities::Export>().visit(
            [&](Capabilities::Export exporter) {
              terminal = true;
              return exporter.expose(workspace<false>());
            },
            [&](Binding::Failure) {
              terminal = query.supports<Capabilities::Export>() ==
                         Binding::Status::Satisfied;
              return terminal ? Binding::Status::Rejected
                              : Binding::Status::Satisfied;
            });
      };
      const auto status = plugin.library.visit(Query(), Receiver(receive));
      if (status != Binding::Status::Satisfied) {
        return reject(
            input, "Terminal did not accept the workspace"_view, plugin.name);
      }
      terminal_count += terminal;
    }
    if (!terminal_count) {
      return reject(input, "No terminal capability was discovered"_view);
    }
    return true;
  }

 private:
  Object<> storage;
  Dynamic::Bytes output;
  // Reverse destruction releases every imported graph before unloading any
  // module whose release callbacks or payload destructors may still execute.
  Dynamic::Vector<BuildModule> modules;
  Dynamic::Vector<BuildMember> members;
};

class BuildImporter {
 public:
  auto get_data() const -> View::Bytes { return "Build"_view; }
  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Capabilities::Import::contract_id ? Binding::Status::Satisfied
                                                   : Binding::Status::Unknown;
  }
  auto bind_interface(Perimortem::System::Uuid id, Storage target) const
      -> Binding::Status {
    if (id == Capabilities::Import::contract_id) {
      return Binding::provide<Capabilities::Import>(
          Capabilities::Import::provide(*this).get_abi(), target);
    }
    return Binding::Status::Unknown;
  }
  template <typename Receiver>
  auto visit(const void* data, const Representation& form, Receiver& receive)
      const -> Binding::Status {
    const auto& expected = Compiled<
        Native<tetrodotoxin_build_input>::reference>::get_representation();
    if (!data || !expected.compatible(form)) {
      return Binding::Status::Rejected;
    }
    const auto& input = *static_cast<const tetrodotoxin_build_input*>(data);
    if (!valid_name(input.output)) {
      reject(input, "Invalid output directory"_view);
      return Binding::Status::Rejected;
    }
    auto& owner = BuildWorkspace::create(view(input.output));
    const bool complete = owner.build(input);
    if (complete) {
      receive(owner.workspace<false>());
    }
    owner.release();
    return complete ? Binding::Status::Satisfied : Binding::Status::Rejected;
  }
};

extern "C" ttx_binding_status ttx_query(
    ttx_semantic_query,
    ttx_query_receiver receiver) {
  if (!receiver.receive) {
    return TTX_BINDING_REJECTED;
  }
  const BuildImporter importer;
  return receiver.receive(
      receiver.source, Abstract::provide(importer).get_query());
}

extern "C" const ttx_representation* tetrodotoxin_build_input_representation() {
  return &Compiled<
      Native<tetrodotoxin_build_input>::reference>::get_representation();
}

extern "C" const ttx_representation*
    tetrodotoxin_build_workspace_representation() {
  return &Binding::representation<Build::Workspace>();
}
