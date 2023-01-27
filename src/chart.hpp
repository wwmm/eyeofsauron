/*
 *  Copyright © 2017-2023 Wellington Wallace
 *
 *  This file is part of Easy Effects.
 *
 *  Easy Effects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Easy Effects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Easy Effects. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <adwaita.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <sigc++/sigc++.h>
#include <algorithm>
#include <deque>
#include <ranges>
#include <string>
#include <vector>
#include "colors.hpp"
#include "tags_app.hpp"
#include "tags_resources.hpp"
#include "ui_helpers.hpp"
#include "util.hpp"

namespace ui::chart {

G_BEGIN_DECLS

#define EE_TYPE_CHART (chart_get_type())

G_DECLARE_FINAL_TYPE(Chart, chart, WW, CHART, GtkWidget)

G_END_DECLS

enum class ChartType { bar, line, dots };

enum class ChartScale { linear, logarithmic };

auto create() -> Chart*;

void queue_draw(Chart* self);

void add_curve(Chart* self);

void remove_curve(Chart* self, const int& curve_index);

void remove_all_curves(Chart* self);

void add_point(Chart* self, const int& curve_index, const double& x, const double& y);

void update(Chart* self);

void reset_zoom(Chart* self);

auto get_curve(Chart* self, const int& index) -> std::tuple<std::deque<double>, std::deque<double>>;

void set_chart_type(Chart* self, const ChartType& value);

void set_chart_scale(Chart* self, const ChartScale& value);

void set_background_color(Chart* self, GdkRGBA color);

void set_axis_labels_color(Chart* self, GdkRGBA color);

void set_line_width(Chart* self, const float& value);

void set_draw_bar_border(Chart* self, const bool& v);

void set_rounded_corners(Chart* self, const bool& v);

void set_fill_bars(Chart* self, const bool& v);

void set_n_x_decimals(Chart* self, const int& v);

void set_n_y_decimals(Chart* self, const int& v);

void set_x_unit(Chart* self, const std::string& value);

void set_y_unit(Chart* self, const std::string& value);

void set_margin(Chart* self, const float& v);

void set_max_points(Chart* self, const size_t& v);

auto get_is_visible(Chart* self) -> bool;

}  // namespace ui::chart
