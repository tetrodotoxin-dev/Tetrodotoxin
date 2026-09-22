// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"

#include "perimortem/graphics/formats/png.hpp"
#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/projection.hpp"
#include "perimortem/graphics/sprite.hpp"
#include "perimortem/graphics/texture_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness GraphicsObjects = {
  .name = "Perimortem::Graphics::Objects"_view,
};

static auto one_pixel_image() -> Image {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel::from_rgba(0x12, 0x34, 0x56, 0x78));
  return Image(Data::take(pixels), 1, 1);
}

static auto finalize_shader(U8*) -> void {}

static const Object<>::Descriptor
    shader_descriptor(sizeof(R32) * 4, alignof(R32), finalize_shader);

alignas(U32) static constexpr U32 graphics_program[] = {0x07230203};

static const Projection graphics_projection = {
  Data::cast<const U8>(graphics_program),
  0,
  sizeof(R32) * 4,
};

static auto create_shader() -> Implementation {
  Object<> object = Object<>::create(shader_descriptor);
  auto shader = Implementation::retain(object, &graphics_projection);
  object.release();
  return shader ? static_cast<Implementation&&>(*shader) : Implementation();
}

PERIMORTEM_UNIT_TEST(GraphicsObjects, image_shares_immutable_pixels) {
  Image empty;
  EXPECT_NOT(empty.is_drawable());
  EXPECT(empty.get_pixels().is_empty());

  Image image = one_pixel_image();
  Image alias = image;
  EXPECT(image.is_drawable());
  EXPECT_EQ(image.get_size_pixels().width, U32(1));
  EXPECT_EQ(image.get_size_pixels().height, U32(1));
  EXPECT_EQ(image.get_pixels().get_data(), alias.get_pixels().get_data());
  image = Image();
  EXPECT_EQ(alias.get_pixel(0, 0).red, U8(0x12));
}

PERIMORTEM_UNIT_TEST(GraphicsObjects, pixel_factories_are_explicit) {
  Pixel grey = Pixel::from_grey(0x22);
  Pixel grey_alpha = Pixel::from_grey_alpha(0x33, 0x44);
  Pixel rgb = Pixel::from_rgb(0x55, 0x66, 0x77);
  Pixel rgba = Pixel::from_rgba(0x88, 0x99, 0xAA, 0xBB);

  EXPECT_EQ(grey.red, U8(0x22));
  EXPECT_EQ(grey.alpha, U8(0xFF));
  EXPECT_EQ(grey_alpha.green, U8(0x33));
  EXPECT_EQ(grey_alpha.alpha, U8(0x44));
  EXPECT_EQ(rgb.blue, U8(0x77));
  EXPECT_EQ(rgb.alpha, U8(0xFF));
  EXPECT_EQ(rgba.red, U8(0x88));
  EXPECT_EQ(rgba.alpha, U8(0xBB));

  Size2D size = {12, 34};
  EXPECT_EQ(size.width, U32(12));
  EXPECT_EQ(size.height, U32(34));
}

PERIMORTEM_UNIT_TEST(GraphicsObjects, decode_reports_success) {
  auto source = Perimortem::System::File::read(
      "validation/data/pngs/checkerboard_2x2.png"_view);
  ASSERT(source);
  Image decoded = Formats::Png::decode(*source);
  ASSERT(decoded.is_drawable());
  EXPECT_EQ(decoded.get_width(), U32(2));
  EXPECT_EQ(decoded.get_height(), U32(2));

  EXPECT_NOT(Formats::Png::decode("not a png"_view).is_drawable());
}

PERIMORTEM_UNIT_TEST(GraphicsObjects, sprite_defaults_and_aliases) {
  Sprite sprite;
  EXPECT_NOT(sprite.get_object().is_empty());
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(sprite.is_visible());
  EXPECT_EQ(sprite.get_z_index(), S64(0));
  EXPECT(sprite.get_material().is_empty());

  Image image = one_pixel_image();
  Texture2D texture(image);
  sprite.set_texture(texture);
  sprite.set_material(create_shader());
  sprite.set_size({64, 32});
  Transform2D transform;
  transform.translation = {12.0, 34.0};
  transform.scale_x = 2.0;
  sprite.set_transform(transform);
  sprite.set_z_index(7);
  ASSERT(sprite.is_drawable());

  Sprite alias = sprite;
  alias.set_visible(False);
  EXPECT_NOT(sprite.is_visible());
  EXPECT_NOT(sprite.is_drawable());
  EXPECT_EQ(sprite.get_transform().translation.x, R64(12.0));
  EXPECT_EQ(sprite.get_size().width, U32(64));
  EXPECT_EQ(sprite.get_material().get_projection(), &graphics_projection);
  EXPECT_EQ(sprite.get_object().get_reservations(), Count(2));
}
