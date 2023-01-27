#pragma once

#include <gdk/gdkrgba.h>
#include <array>

namespace ui::colors {

constexpr auto list = std::to_array(
    {GdkRGBA{240.0F / 255.0F, 228.0F / 255.0F, 66.0F / 255.0F, 1.0F},
     GdkRGBA{230.0F / 255.0F, 159.0F / 255.0F, 0.0F, 1.0F}, GdkRGBA{213.0F / 255.0F, 94.0F / 255.0F, 0.0F, 1.0F},
     GdkRGBA{204.0F / 255.0F, 121.0F / 255.0F, 167.0F / 255.0F, 1.0F},
     GdkRGBA{0.0F, 114.0F / 255.0F, 178.0F / 255.0F, 1.0F},
     GdkRGBA{86.0F / 255.0F, 180.0F / 255.0F, 233.0F / 255.0F, 1.0F},
     GdkRGBA{0.0F, 158.0F / 255.0F, 115.0F / 255.0F, 1.0F}, GdkRGBA{0.0F, 0.0F, 0.0F, 1.0F}});

}  // namespace ui::colors