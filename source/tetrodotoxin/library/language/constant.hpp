// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/constant.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Language {

// Constant is an immutable semantic fact already in normal form. It defines no
// parser, operator set, evaluation engine, or lowering representation. Concrete
// value domains expose their payload through derived contracts without
// extending a central tag.
//
// Equality includes resolved Type identity as well as the derived value,
// preserving the distinction between equal bits interpreted by different
// Types.
class Constant : public Tetrodotoxin::Source::Constant, public Model::Pack {
 public:
  TTX_CONTRACT(Constant, Tetrodotoxin::Source::Constant);

  auto bind_interface(Perimortem::System::Uuid requested) const
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Negotiation::Binding,
          Ttx::Semantic::Negotiation::Binding::Failure> override;

  // Concrete Constant domains override this with the canonical spelling of
  // the fact they represent. The Type name remains only a safe fallback for
  // external Constant domains that have not selected a value spelling.
  TTX_NAME(get_type().get_name());

  // A Constant is a value rather than an authored declaration. When it is
  // stored under a documented name, that prose belongs to the Addressable.
  TTX_EMPTY_DOCUMENTATION();

  virtual constexpr auto get_type() const -> const Model::Type& override = 0;
  virtual constexpr auto equals(const Constant& rhs) const -> Bool = 0;

  constexpr auto get_result() const -> const Tetrodotoxin::Source::Abstract& override {
    return *this;
  }

  auto get_identity() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override {
    return *this;
  }

  constexpr auto get_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
    return anchor;
  }

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor&,
      const Tetrodotoxin::Source::Abstract&,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> = {})
      -> Bool override {
    return True;
  }

  auto link_restored(
      const Tetrodotoxin::Source::Abstract&,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&>)
      -> Bool override {
    return True;
  }

  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override {
    return output_layout;
  }

  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto is_complete() const -> Bool override { return True; }

  auto fits(const Tetrodotoxin::Source::Layout& target) const -> Bool override {
    return Model::Pack::fits(target);
  }

  auto fits(const Tetrodotoxin::Source::Type& target) const -> Bool override {
    const Tetrodotoxin::Source::Abstract& source = get_type().resolve();
    auto type = source.select<Model::Type>();
    return type && type->get_layout().fits(target.get_layout());
  }

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor&) -> void override {}

  constexpr auto operator==(const Constant& rhs) const -> Bool {
    return equals(rhs);
  }
  constexpr auto operator!=(const Constant& rhs) const -> Bool {
    return !equals(rhs);
  }

 protected:
  template <typename type, typename builder_type>
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Constant, type));
    Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> source(anchor);
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  template <typename type, typename builder_type>
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Constant, type));
    Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> source;
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  constexpr explicit Constant(
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : anchor(anchor), output_layout(*this, 1) {}

  constexpr auto has_same_type(const Constant& rhs) const -> Bool {
    const Tetrodotoxin::Source::Abstract& lhs_type = get_type().resolve();
    const Tetrodotoxin::Source::Abstract& rhs_type = rhs.get_type().resolve();
    return lhs_type.is<Model::Type>() && rhs_type.is<Model::Type>() &&
           &lhs_type == &rhs_type;
  }

  static auto have_equal_values(
      const Model::Pack& left,
      const Model::Pack& right) -> Bool;

 private:
  Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor;
  Tetrodotoxin::Source::Layouts::Ranged output_layout;
};

}  // namespace Tetrodotoxin::Library::Language
