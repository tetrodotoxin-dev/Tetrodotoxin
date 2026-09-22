// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"

#include "perimortem/graphics/sampler_2d.hpp"
#include "perimortem/graphics/texture_2d.hpp"

namespace Perimortem::Graphics::Frame {

// Resource keeps one worker-local Image identity and its sampling value alive
// for the immutable frame that references them.
class Resource {
 public:
  Resource() = default;
  explicit Resource(Perimortem::Core::Object<> object);
  static auto retain_texture(const Perimortem::Graphics::Texture2D& texture)
      -> Resource;
  Resource(const Resource& source);
  Resource(Resource&& source);
  ~Resource();

  auto operator=(const Resource& source) -> Resource&;
  auto operator=(Resource&& source) -> Resource&;

  constexpr auto is_empty() const -> Bool { return object.is_empty(); }
  constexpr auto get_object() const -> Perimortem::Core::Object<> {
    return object;
  }
  constexpr auto get_sampler() const -> Perimortem::Graphics::Sampler2D {
    return sampler;
  }
  constexpr auto matches(const Resource& other) const -> Bool {
    return object.get_payload() == other.object.get_payload() &&
           sampler == other.sampler;
  }
  auto get_reservations() const -> Count;

 private:
  Perimortem::Core::Object<> object;
  Perimortem::Graphics::Sampler2D sampler;
};

}  // namespace Perimortem::Graphics::Frame
