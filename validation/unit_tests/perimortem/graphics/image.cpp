// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/sampler_2d.hpp"
#include "perimortem/graphics/texture_2d.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/graphics/frame/resource.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness GraphicsImage = {
  .name = "Graphics::Image"_view,
};

PERIMORTEM_UNIT_TEST(GraphicsImage, reads_content_without_sampling_policy) {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel::from_rgba(0x11, 0x22, 0x33, 0x44));
  Image image(Data::take(pixels), 1, 1);

  const Pixel horizontal_edge = image.get_pixel(1, 0);
  const Pixel vertical_edge = image.get_pixel(0, 1);
  EXPECT_EQ(horizontal_edge.red, U8(0));
  EXPECT_EQ(horizontal_edge.alpha, U8(0));
  EXPECT_EQ(vertical_edge.red, U8(0));
  EXPECT_EQ(vertical_edge.alpha, U8(0));
}

PERIMORTEM_UNIT_TEST(GraphicsImage, texture_values_share_one_image) {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel::from_rgba(0x11, 0x22, 0x33, 0x44));
  pixels.emplace(Pixel::from_rgba(0x55, 0x66, 0x77, 0x88));
  Image image(Data::take(pixels), 2, 1);
  Texture2D wrapped(
      image,
      Sampler2D(
          Sampler2D::Addressing::Wrap, Sampler2D::Filtering::Linear));
  Texture2D clamped(
      image,
      Sampler2D(
          Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));

  EXPECT_EQ(
      wrapped.get_image().get_object().get_payload(),
      clamped.get_image().get_object().get_payload());
  EXPECT(
      wrapped.get_sampler().get_addressing() ==
      Sampler2D::Addressing::Wrap);
  EXPECT(
      clamped.get_sampler().get_addressing() ==
      Sampler2D::Addressing::Clamp);
  EXPECT(
      wrapped.get_sampler().get_filtering() ==
      Sampler2D::Filtering::Linear);
  EXPECT(
      clamped.get_sampler().get_filtering() ==
      Sampler2D::Filtering::Nearest);

  auto wrapped_resource = Frame::Resource::retain_texture(wrapped);
  auto clamped_resource = Frame::Resource::retain_texture(clamped);
  EXPECT_EQ(
      wrapped_resource.get_object().get_payload(),
      clamped_resource.get_object().get_payload());
  EXPECT_NOT(wrapped_resource.matches(clamped_resource));
  EXPECT_EQ(wrapped_resource.get_reservations(), Count(5));
}
