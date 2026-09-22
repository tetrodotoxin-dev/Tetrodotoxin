// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "Perimortem.Graphics/1.0/c_abi.h"

static const uint8_t red_png[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
  0x0d, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
  0x1f, 0x00, 0x05, 0x00, 0x01, 0xff, 0x56, 0xc7, 0x2f, 0x0d, 0x00, 0x00,
  0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

int main(void) {
  ttx_perimortem_graphics_Pixel rgb =
      TTX_FUNC_Perimortem_2eGraphics__Pixel__Pixel__from_5frgb_static(
          0x11, 0x22, 0x33);
  ttx_perimortem_graphics_Pixel rgba =
      TTX_FUNC_Perimortem_2eGraphics__Pixel__Pixel__from_5frgba_static(
          0x44, 0x55, 0x66, 0x77);
  if (rgb.red != 0x11 || rgb.alpha != 0xff || rgba.blue != 0x66 ||
      rgba.alpha != 0x77) {
    return 1;
  }

  ttx_perimortem_graphics_PNGSource_View_5bU8_5d bytes = {
    red_png,
    sizeof(red_png),
  };
  ttx_perimortem_graphics_PNGSource_Option_5bImage_5d decoded =
      TTX_FUNC_Perimortem_2eGraphics__PNGSource__PNG__decode_static(bytes);
  if (!decoded) {
    return 2;
  }
  ttx_perimortem_graphics_Image image = decoded;

  ttx_perimortem_graphics_Image_View_5bPixel_5d pixels =
      TTX_FUNC_Perimortem_2eGraphics__Image__Image__get_5fpixels_self(&image);
  int result = 0;
  if (pixels.size != 1 || pixels.data[0].red != 0xff ||
      pixels.data[0].green != 0x00 ||
      pixels.data[0].blue != 0x00 || pixels.data[0].alpha != 0xff) {
    result = 3;
  }

  ttx_perimortem_graphics_Texture2D texture = {
    image,
    {0, 1},
  };
  ttx_perimortem_math_Vec2D origin = {0.0f, 0.0f};
  ttx_perimortem_math_Vec4D sampled =
      TTX_FUNC_Perimortem_2eGraphics__Texture2D__Texture2D__sample_self(
          &texture, origin);
  if (sampled.x < 0.999f || sampled.x > 1.001f || sampled.y != 0.0f ||
      sampled.z != 0.0f || sampled.w < 0.999f || sampled.w > 1.001f) {
    result = 4;
  }

  ttx_perimortem_math_Vec2D outside = {2.0f, 2.0f};
  sampled =
      TTX_FUNC_Perimortem_2eGraphics__Texture2D__Texture2D__sample_self(
          &texture, outside);
  if (sampled.x != 0.0f || sampled.y != 0.0f || sampled.z != 0.0f ||
      sampled.w != 0.0f) {
    result = 5;
  }

  perimortem_core_object_release(image);
  return result;
}
