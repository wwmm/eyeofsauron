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

#include "chart.hpp"

namespace {

struct DataRange {
  double xmin = 0.0;
  double xmax = 0.0;
  double ymin = 0.0;
  double ymax = 0.0;

  [[nodiscard]] auto contains(const double& point_x, const double& point_y) const -> bool {
    return point_x >= xmin && point_x <= xmax && point_y >= ymin && point_y <= ymax;
  }
};

struct ZoomRectangle {
  double x = 0;
  double y = 0;
  double width = 0;
  double height = 0;
};

bool primary_mouse_button_pressed = false;
bool secondary_mouse_button_pressed = false;

ZoomRectangle zoom_rectangle;

GdkRGBA zoom_rect_color = GdkRGBA{0.0F, 0.0F, 1.0F, 0.5F};

}  // namespace

namespace ui::chart {

using namespace std::string_literals;

/*
  As multiple charts may be created at the same time we can not define some variables as globals because one chart
  instance will conflict with the others
*/

struct Data {
 public:
  bool draw_bar_border = false, fill_bars = false, is_visible = false, rounded_corners = false, show_zoom = false;

  int x_axis_height = 0, y_axis_width = 0, n_x_decimals = 0, n_y_decimals = 0;

  double mouse_y = 0.0, mouse_x = 0.0, margin = 0.0, line_width = 0.0;

  double x_min_log = 0.0, x_max_log = 0.0;

  size_t max_points = 300;

  double global_x_min = 0, global_x_max = 0;

  double global_y_min = 0, global_y_max = 0;

  double global_x_min_log = 0, global_x_max_log = 0;

  ChartType chart_type{};

  ChartScale chart_scale{};

  GdkRGBA background_color{}, color_axis_labels{}, gradient_color{};

  std::string x_unit, y_unit;

  DataRange data_range;

  std::vector<double> x_axis_log;

  std::vector<std::deque<double>> curves_x, curves_y;
};

struct _Chart {
  GtkBox parent_instance;

  GtkEventController* controller_motion;

  GtkGestureSingle* gesture_single;

  Data* data;
};

G_DEFINE_TYPE(Chart, chart, GTK_TYPE_WIDGET)

auto get_global_min_and_max(const std::vector<std::deque<double>>& axis) -> std::tuple<double, double> {
  double global_min = 0;
  double global_max = 0;

  for (size_t n = 0; n < axis.size(); n++) {
    double min = *std::min_element(axis[n].begin(), axis[n].end());
    double max = *std::max_element(axis[n].begin(), axis[n].end());

    if (n == 0) {
      global_min = min;
      global_max = max;
    } else {
      global_min = (min < global_min) ? min : global_min;
      global_max = (max > global_max) ? max : global_max;
    }
  }

  return {global_min, global_max};
}

void add_curve(Chart* self) {
  std::deque<double> x;
  std::deque<double> y;

  self->data->curves_x.emplace_back(x);
  self->data->curves_y.emplace_back(y);
}

void remove_curve(Chart* self, const int& curve_index) {
  self->data->curves_x.erase(self->data->curves_x.begin() + curve_index);
  self->data->curves_y.erase(self->data->curves_y.begin() + curve_index);
}

void remove_all_curves(Chart* self) {
  self->data->curves_x.clear();
  self->data->curves_y.clear();
}

void add_point(Chart* self, const int& curve_index, const double& x, const double& y) {
  if (self->data->curves_x.empty() || static_cast<size_t>(curve_index) + 1 > self->data->curves_x.size()) {
    return;
  }

  self->data->curves_x[curve_index].emplace_back(x);
  self->data->curves_y[curve_index].emplace_back(y);

  while (self->data->curves_x[curve_index].size() > self->data->max_points) {
    self->data->curves_x[curve_index].pop_front();
    self->data->curves_y[curve_index].pop_front();
  }
}

void update(Chart* self) {
  std::tie(self->data->global_x_min, self->data->global_x_max) = get_global_min_and_max(self->data->curves_x);
  std::tie(self->data->global_y_min, self->data->global_y_max) = get_global_min_and_max(self->data->curves_y);

  self->data->global_x_min_log = std::log10(self->data->global_x_min);
  self->data->global_x_max_log = std::log10(self->data->global_x_max);

  self->data->data_range.xmin = self->data->global_x_min;
  self->data->data_range.xmax = self->data->global_x_max;
  self->data->data_range.ymin = self->data->global_y_min;
  self->data->data_range.ymax = self->data->global_y_max;

  queue_draw(self);
}

void reset_zoom(Chart* self) {
  self->data->data_range.xmin = self->data->global_x_min;
  self->data->data_range.xmax = self->data->global_x_max;
  self->data->data_range.ymin = self->data->global_y_min;
  self->data->data_range.ymax = self->data->global_y_max;

  queue_draw(self);
}

auto get_curve(Chart* self, const int& index) -> std::tuple<std::deque<double>, std::deque<double>> {
  std::deque<double> x;
  std::deque<double> y;

  if (index + 1 <= static_cast<int>(self->data->curves_x.size())) {
    x = self->data->curves_x[index];
    y = self->data->curves_y[index];
  }

  return {x, y};
}

void set_chart_type(Chart* self, const ChartType& value) {
  if (self->data == nullptr) {
    return;
  }

  self->data->chart_type = value;
}

void set_chart_scale(Chart* self, const ChartScale& value) {
  if (self->data == nullptr) {
    return;
  }

  self->data->chart_scale = value;
}

void set_background_color(Chart* self, GdkRGBA color) {
  if (self->data == nullptr) {
    return;
  }

  self->data->background_color = color;
}

void set_axis_labels_color(Chart* self, GdkRGBA color) {
  if (self->data == nullptr) {
    return;
  }

  self->data->color_axis_labels = color;
}

void set_line_width(Chart* self, const float& value) {
  if (self->data == nullptr) {
    return;
  }

  self->data->line_width = value;
}

void set_draw_bar_border(Chart* self, const bool& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->draw_bar_border = v;
}

void set_rounded_corners(Chart* self, const bool& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->rounded_corners = v;
}

void set_fill_bars(Chart* self, const bool& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->fill_bars = v;
}

void set_n_x_decimals(Chart* self, const int& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->n_x_decimals = v;
}

void set_n_y_decimals(Chart* self, const int& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->n_y_decimals = v;
}

void set_x_unit(Chart* self, const std::string& value) {
  if (self->data == nullptr) {
    return;
  }

  self->data->x_unit = value;
}

void set_y_unit(Chart* self, const std::string& value) {
  if (self->data == nullptr) {
    return;
  }

  self->data->y_unit = value;
}

void set_margin(Chart* self, const float& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->margin = v;
}

void set_max_points(Chart* self, const size_t& v) {
  if (self->data == nullptr) {
    return;
  }

  self->data->max_points = v;
}

auto get_is_visible(Chart* self) -> bool {
  return (self->data != nullptr) ? self->data->is_visible : false;
}

void on_pointer_motion(GtkEventControllerMotion* controller, double x, double y, Chart* self) {
  const auto width = static_cast<double>(gtk_widget_get_allocated_width(GTK_WIDGET(self)));
  const auto height = static_cast<double>(gtk_widget_get_allocated_height(GTK_WIDGET(self)));

  const auto usable_height = height - self->data->margin * height - self->data->x_axis_height;

  if (y < height - self->data->x_axis_height && y > self->data->margin * height &&
      x > (self->data->margin * width + self->data->y_axis_width) && x < width - self->data->margin * width) {
    // At least for now the y axis is always linear

    self->data->mouse_y =
        (usable_height - y) / usable_height * (self->data->data_range.ymax - self->data->data_range.ymin) +
        self->data->data_range.ymin;

    switch (self->data->chart_scale) {
      case ChartScale::logarithmic: {
        const double mouse_x_log = (x - self->data->margin * width) / (width - 2 * self->data->margin * width) *
                                       (self->data->global_x_max_log - self->data->global_x_min_log) +
                                   self->data->global_x_min_log;

        self->data->mouse_x = std::pow(10.0, mouse_x_log);  // exp10 does not exist on FreeBSD

        break;
      }
      case ChartScale::linear: {
        self->data->mouse_x = (x - self->data->margin * width - self->data->y_axis_width) /
                                  (width - 2 * self->data->margin * width - self->data->y_axis_width) *
                                  (self->data->data_range.xmax - self->data->data_range.xmin) +
                              self->data->data_range.xmin;

        break;
      }
    }

    // preparing the zoom

    if (primary_mouse_button_pressed) {
      zoom_rectangle.width = x - zoom_rectangle.x;
      zoom_rectangle.height = y - zoom_rectangle.y;
    }

    queue_draw(self);
  }
}

void on_gesture_click_pressed(GtkGestureClick* gesture_object, gint n_press, gdouble x, gdouble y, gpointer user_data) {
  auto* self = WW_CHART(user_data);

  switch (gtk_gesture_single_get_current_button(self->gesture_single)) {
    case GDK_BUTTON_PRIMARY: {
      primary_mouse_button_pressed = true;

      zoom_rectangle.x = x;
      zoom_rectangle.y = y;
      zoom_rectangle.width = 0.0;
      zoom_rectangle.height = 0.0;

      break;
    }
    case GDK_BUTTON_SECONDARY: {
      secondary_mouse_button_pressed = true;

      break;
    }
  }
}

void on_gesture_click_released(GtkGestureClick* gesture_object,
                               gint n_press,
                               gdouble x,
                               gdouble y,
                               gpointer user_data) {
  auto* self = WW_CHART(user_data);

  switch (gtk_gesture_single_get_current_button(self->gesture_single)) {
    case GDK_BUTTON_PRIMARY: {
      primary_mouse_button_pressed = false;

      zoom_rectangle.width = x - zoom_rectangle.x;
      zoom_rectangle.height = y - zoom_rectangle.y;

      if (std::abs(zoom_rectangle.width) == 0 || std::abs(zoom_rectangle.height) == 0) {
        return;
      }

      if (zoom_rectangle.width < 0) {
        zoom_rectangle.x = zoom_rectangle.width + zoom_rectangle.x;

        zoom_rectangle.width *= -1;
      }

      if (zoom_rectangle.height < 0) {
        zoom_rectangle.y = zoom_rectangle.height + zoom_rectangle.y;

        zoom_rectangle.height *= -1;
      }

      auto window_width = gtk_widget_get_width(GTK_WIDGET(self));
      auto window_height = gtk_widget_get_height(GTK_WIDGET(self));

      double usable_width =
          window_width - 2.0 * (self->data->line_width + self->data->margin * window_width) - self->data->y_axis_width;

      double usable_height = (window_height - self->data->margin * window_height) - self->data->x_axis_height;

      double xmin = self->data->data_range.xmin;
      double xmax = self->data->data_range.xmax;
      double ymin = self->data->data_range.ymin;
      double ymax = self->data->data_range.ymax;

      auto offset_x = self->data->margin * window_width + self->data->y_axis_width + self->data->line_width;

      auto offset_y = self->data->margin * window_height + self->data->x_axis_height + self->data->line_width;

      self->data->data_range.xmin = xmin + (xmax - xmin) * (zoom_rectangle.x - offset_x) / usable_width;

      self->data->data_range.xmax =
          xmin + (xmax - xmin) * (zoom_rectangle.x + zoom_rectangle.width - offset_x) / usable_width;

      self->data->data_range.ymin =
          ymin + (ymax - ymin) * (window_height - offset_y - zoom_rectangle.y - zoom_rectangle.height) / usable_height;

      self->data->data_range.ymax =
          ymin + (ymax - ymin) * (window_height - offset_y - zoom_rectangle.y) / usable_height;

      return;
    }
    case GDK_BUTTON_SECONDARY: {
      secondary_mouse_button_pressed = false;

      return;
    }
  }
}

auto draw_unit(Chart* self, GtkSnapshot* snapshot, const float& width, const float& height, const std::string& unit) {
  auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), unit.c_str());

  auto* description = pango_font_description_from_string("monospace bold");

  pango_layout_set_font_description(layout, description);
  pango_font_description_free(description);

  int text_width = 0;
  int text_height = 0;

  pango_layout_get_pixel_size(layout, &text_width, &text_height);

  gtk_snapshot_save(snapshot);

  auto point = GRAPHENE_POINT_INIT(static_cast<float>(width - text_width), static_cast<float>(height - text_height));

  gtk_snapshot_translate(snapshot, &point);

  gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

  gtk_snapshot_restore(snapshot);

  g_object_unref(layout);
}

auto draw_x_labels(Chart* self, GtkSnapshot* snapshot, const graphene_rect_t& widget_rectangle) -> int {
  const auto width = widget_rectangle.size.width;
  const auto height = widget_rectangle.size.height;

  double labels_offset = 0.1 * width;

  int n_x_labels =
      static_cast<int>(std::ceil((width - 2 * self->data->margin * width - self->data->y_axis_width) / labels_offset)) +
      1;

  if (n_x_labels < 2) {
    return 0;
  }

  /*
    Correcting the offset based on the final n_x_labels value
  */

  labels_offset =
      (width - 2.0 * self->data->margin * width - self->data->y_axis_width) / static_cast<float>(n_x_labels - 1);

  std::vector<double> labels;

  switch (self->data->chart_scale) {
    case ChartScale::logarithmic: {
      labels = util::logspace(self->data->data_range.xmin, self->data->data_range.xmax, n_x_labels);

      break;
    }
    case ChartScale::linear: {
      labels = util::linspace(self->data->data_range.xmin, self->data->data_range.xmax, n_x_labels);

      break;
    }
  }

  draw_unit(self, snapshot, width, height, " "s + self->data->x_unit + " "s);

  if (labels.empty()) {
    return 0;
  }

  /*
    There is no space left in the window to show the last label. So we skip it.
    All labels are enclosed by whitespaces to not stick the first and the final
    at window borders.
  */

  for (size_t n = 0U; n < labels.size() - 1U; n++) {
    const auto msg = fmt::format(ui::get_user_locale(), " {0:.{1}Lf} ", labels[n], self->data->n_x_decimals);

    auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), msg.c_str());

    auto* description = pango_font_description_from_string("monospace");

    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);

    int text_width = 0;
    int text_height = 0;

    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    gtk_snapshot_save(snapshot);

    auto point = GRAPHENE_POINT_INIT(
        static_cast<float>(self->data->margin * width + self->data->y_axis_width + n * labels_offset),
        static_cast<float>(height - text_height));

    gtk_snapshot_translate(snapshot, &point);

    gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

    gtk_snapshot_restore(snapshot);

    g_object_unref(layout);

    if (n == labels.size() - 2U) {
      return text_height;
    }
  }

  return 0;
}

auto draw_y_labels(Chart* self, GtkSnapshot* snapshot, const graphene_rect_t& widget_rectangle) -> int {
  const auto width = widget_rectangle.size.width;
  const auto height = widget_rectangle.size.height;

  double labels_offset = 0.1 * height;

  int n_y_labels = static_cast<int>(std::ceil((height - 2 * self->data->margin * height - self->data->x_axis_height) /
                                              labels_offset)) +
                   1;

  if (n_y_labels < 2) {
    return 0;
  }

  /*
    Correcting the offset based on the final n_x_labels value
  */

  labels_offset =
      (height - 2.0 * self->data->margin * height - self->data->x_axis_height) / static_cast<float>(n_y_labels - 1);

  std::vector<double> labels;

  switch (self->data->chart_scale) {
    case ChartScale::logarithmic: {
      labels = util::logspace(self->data->data_range.ymin, self->data->data_range.ymax, n_y_labels);

      break;
    }
    case ChartScale::linear: {
      labels = util::linspace(self->data->data_range.ymin, self->data->data_range.ymax, n_y_labels);

      break;
    }
  }

  // reversing because the drawing origin is at the left top corner

  std::ranges::reverse(labels);

  auto draw_value = [&](const double& value, const double& offset) {
    const auto msg = fmt::format(ui::get_user_locale(), " {0:.{1}Lf} ", value, self->data->n_y_decimals);

    auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), msg.c_str());

    auto* description = pango_font_description_from_string("monospace");

    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);

    int text_width = 0;
    int text_height = 0;

    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    gtk_snapshot_save(snapshot);

    auto point = GRAPHENE_POINT_INIT(static_cast<float>(self->data->margin * width),
                                     static_cast<float>(self->data->margin * height + offset));

    gtk_snapshot_translate(snapshot, &point);

    gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

    gtk_snapshot_restore(snapshot);

    g_object_unref(layout);

    return text_width;
  };

  if (labels.empty()) {  // it can happen when ymin = ymax
    auto text_width = draw_value(self->data->data_range.ymax, 0.0);

    return text_width;
  }

  /*
    There is no space left in the window to show the last label. So we skip it.
    All labels are enclosed by whitespaces to not stick the first and the final
    at window borders.
  */

  for (size_t n = 0U; n < labels.size(); n++) {
    auto text_width = draw_value(labels[n], static_cast<double>(n) * labels_offset);

    if (n == labels.size() - 1U) {
      return text_width;
    }
  }

  return 0;
}

auto draw_curves(Chart* self, GtkSnapshot* snapshot, const graphene_rect_t& widget_rectangle) {
  if (self->data->curves_x.empty() || self->data->curves_y.empty()) {
    return;
  }

  const auto width = widget_rectangle.size.width;
  const auto height = widget_rectangle.size.height;

  // needs to be improved
  double xrange = (self->data->data_range.xmax == self->data->data_range.xmin)
                      ? 1
                      : self->data->data_range.xmax - self->data->data_range.xmin;
  double yrange = (self->data->data_range.ymax == self->data->data_range.ymin)
                      ? 1
                      : self->data->data_range.ymax - self->data->data_range.ymin;

  std::array<float, 4> border_width = {
      static_cast<float>(self->data->line_width), static_cast<float>(self->data->line_width),
      static_cast<float>(self->data->line_width), static_cast<float>(self->data->line_width)};

  float radius = (self->data->rounded_corners) ? 5.0F : 0.0F;

  std::vector<double> x_axis;
  std::vector<double> y_axis;
  std::vector<double> objects_x;

  for (size_t nc = 0; nc < self->data->curves_x.size(); nc++) {
    auto xdeque = self->data->curves_x[nc];
    auto ydeque = self->data->curves_y[nc];

    if (xdeque.empty() || ydeque.empty()) {
      continue;
    }

    if (nc >= ui::colors::list.size()) {
      return;
    }

    auto n_points = xdeque.size();

    x_axis.resize(n_points);
    y_axis.resize(n_points);
    objects_x.resize(n_points);

    std::copy(xdeque.begin(), xdeque.end(), x_axis.begin());
    std::copy(ydeque.begin(), ydeque.end(), y_axis.begin());

    if (std::fabs(self->data->data_range.xmax - self->data->data_range.xmin) < 0.00001) {
      std::ranges::fill(x_axis, 1.0);
    } else {
      // making each x value a number between 0 and 1

      std::ranges::for_each(x_axis, [&](auto& v) { v = (v - self->data->data_range.xmin) / xrange; });
    }

    if (std::fabs(self->data->data_range.ymax - self->data->data_range.ymin) < 0.00001) {
      std::ranges::fill(y_axis, 1.0);
    } else {
      // making each y value a number between 0 and 1

      std::ranges::for_each(y_axis, [&](auto& v) { v = (v - self->data->data_range.ymin) / yrange; });
    }

    double usable_width =
        width - 2.0 * (self->data->line_width + self->data->margin * width) - self->data->y_axis_width;

    double usable_height = height - self->data->margin * height - self->data->x_axis_height - self->data->line_width;

    switch (self->data->chart_scale) {
      case ChartScale::logarithmic: {
        for (size_t n = 0U; n < n_points; n++) {
          objects_x[n] = usable_width * self->data->x_axis_log[n] + self->data->line_width +
                         self->data->margin * width + self->data->y_axis_width;
        }

        break;
      }
      case ChartScale::linear: {
        for (size_t n = 0U; n < n_points; n++) {
          objects_x[n] =
              usable_width * x_axis[n] + self->data->line_width + self->data->margin * width + self->data->y_axis_width;
        }

        break;
      }
    }

    auto color = ui::colors::list[nc];

    auto border_color = std::to_array({color, color, color, color});

    switch (self->data->chart_type) {
      case ChartType::bar: {
        double dw = width / static_cast<double>(n_points);

        for (uint n = 0U; n < n_points; n++) {
          double bar_height = usable_height * y_axis[n];

          double rect_x = objects_x[n];
          double rect_y = self->data->margin * height + usable_height - bar_height;
          double rect_height = bar_height;
          double rect_width = dw;

          if (self->data->draw_bar_border) {
            rect_width -= self->data->line_width;
          }

          auto bar_rectangle = GRAPHENE_RECT_INIT(static_cast<float>(rect_x), static_cast<float>(rect_y),
                                                  static_cast<float>(rect_width), static_cast<float>(rect_height));

          GskRoundedRect outline;

          gsk_rounded_rect_init_from_rect(&outline, &bar_rectangle, radius);

          if (self->data->fill_bars) {
            gtk_snapshot_push_rounded_clip(snapshot, &outline);

            gtk_snapshot_append_color(snapshot, &color, &outline.bounds);

            gtk_snapshot_pop(snapshot);
          } else {
            gtk_snapshot_append_border(snapshot, &outline, border_width.data(), border_color.data());
          }
        }

        break;
      }
      case ChartType::dots: {
        double dw = width / static_cast<double>(n_points);

        usable_height -= radius;  // this avoids the dots being drawn over the axis label

        for (uint n = 0U; n < n_points; n++) {
          double dot_y = usable_height * y_axis[n];

          double rect_x = objects_x[n];
          double rect_y = self->data->margin * height + radius + usable_height - dot_y;
          double rect_width = dw;

          if (self->data->draw_bar_border) {
            rect_width -= self->data->line_width;
          }

          auto rectangle = GRAPHENE_RECT_INIT(static_cast<float>(rect_x - radius), static_cast<float>(rect_y - radius),
                                              static_cast<float>(rect_width), static_cast<float>(rect_width));

          GskRoundedRect outline;

          gsk_rounded_rect_init_from_rect(&outline, &rectangle, radius);

          if (self->data->fill_bars) {
            gtk_snapshot_push_rounded_clip(snapshot, &outline);

            gtk_snapshot_append_color(snapshot, &color, &outline.bounds);

            gtk_snapshot_pop(snapshot);
          } else {
            gtk_snapshot_append_border(snapshot, &outline, border_width.data(), border_color.data());
          }
        }

        break;
      }
      case ChartType::line: {
        auto* ctx = gtk_snapshot_append_cairo(snapshot, &widget_rectangle);

        cairo_set_source_rgba(ctx, static_cast<double>(color.red), static_cast<double>(color.green),
                              static_cast<double>(color.blue), static_cast<double>(color.alpha));

        if (self->data->fill_bars) {
          cairo_move_to(ctx, self->data->margin * width, self->data->margin * height + usable_height);
        } else {
          const auto point_height = y_axis.front() * usable_height;

          cairo_move_to(ctx, objects_x.front(), self->data->margin * height + usable_height - point_height);
        }

        for (uint n = 0U; n < n_points - 1U; n++) {
          const auto next_point_height = y_axis[n + 1U] * usable_height;

          cairo_line_to(ctx, objects_x[n + 1U], self->data->margin * height + usable_height - next_point_height);
        }

        if (self->data->fill_bars) {
          cairo_line_to(ctx, objects_x.back(), self->data->margin * height + usable_height);

          cairo_move_to(ctx, objects_x.back(), self->data->margin * height + usable_height);

          cairo_close_path(ctx);
        }

        cairo_set_line_width(ctx, self->data->line_width);

        if (self->data->fill_bars) {
          cairo_fill(ctx);
        } else {
          cairo_stroke(ctx);
        }

        cairo_destroy(ctx);

        break;
      }
    }
  }
}

auto draw_axes(Chart* self, GtkSnapshot* snapshot, const graphene_rect_t& widget_rectangle) {
  const auto width = widget_rectangle.size.width;
  const auto height = widget_rectangle.size.height;

  auto* ctx = gtk_snapshot_append_cairo(snapshot, &widget_rectangle);

  auto color = self->data->color_axis_labels;

  cairo_set_source_rgba(ctx, static_cast<double>(color.red), static_cast<double>(color.green),
                        static_cast<double>(color.blue), static_cast<double>(color.alpha));

  // drawing the x axis

  cairo_move_to(ctx, self->data->margin * width + self->data->y_axis_width,
                height - static_cast<float>(self->data->x_axis_height));

  cairo_line_to(ctx, width - self->data->margin * width, height - static_cast<float>(self->data->x_axis_height));

  cairo_set_line_width(ctx, self->data->line_width);

  // drawing the y axis

  cairo_move_to(ctx, self->data->margin * width + self->data->y_axis_width,
                height - static_cast<float>(self->data->x_axis_height));

  cairo_line_to(ctx, self->data->margin * width + self->data->y_axis_width, self->data->margin * height);

  cairo_stroke(ctx);

  cairo_destroy(ctx);
}

void snapshot(GtkWidget* widget, GtkSnapshot* snapshot) {
  auto* self = WW_CHART(widget);

  auto width = gtk_widget_get_width(widget);
  auto height = gtk_widget_get_height(widget);

  auto widget_rectangle = GRAPHENE_RECT_INIT(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height));

  gtk_snapshot_append_color(snapshot, &self->data->background_color, &widget_rectangle);

  self->data->x_axis_height = draw_x_labels(self, snapshot, widget_rectangle);

  self->data->y_axis_width = draw_y_labels(self, snapshot, widget_rectangle);

  draw_axes(self, snapshot, widget_rectangle);

  draw_curves(self, snapshot, widget_rectangle);

  if (gtk_event_controller_motion_contains_pointer(GTK_EVENT_CONTROLLER_MOTION(self->controller_motion)) != 0) {
    // We leave a withespace at the end to not stick the string at the window border.
    const auto msg = fmt::format(ui::get_user_locale(), "t = {0:.{1}Lf} {2} y = {3:.{4}Lf} {5} ", self->data->mouse_x,
                                 self->data->n_x_decimals, self->data->x_unit, self->data->mouse_y,
                                 self->data->n_y_decimals, self->data->y_unit);

    auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), msg.c_str());

    auto* description = pango_font_description_from_string("monospace bold");

    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);

    int text_width = 0;
    int text_height = 0;

    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    gtk_snapshot_save(snapshot);

    auto point = GRAPHENE_POINT_INIT(width - static_cast<float>(text_width), 0.0F);

    gtk_snapshot_translate(snapshot, &point);

    gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

    gtk_snapshot_restore(snapshot);

    g_object_unref(layout);
  }

  if (primary_mouse_button_pressed) {
    auto rectangle =
        GRAPHENE_RECT_INIT(static_cast<float>(zoom_rectangle.x), static_cast<float>(zoom_rectangle.y),
                           static_cast<float>(zoom_rectangle.width), static_cast<float>(zoom_rectangle.height));

    gtk_snapshot_append_color(snapshot, &zoom_rect_color, &rectangle);
  }
}

void unroot(GtkWidget* widget) {
  auto* self = WW_CHART(widget);

  self->data->is_visible = false;

  GTK_WIDGET_CLASS(chart_parent_class)->unmap(widget);
}

void finalize(GObject* object) {
  auto* self = WW_CHART(object);

  delete self->data;

  self->data = nullptr;

  util::debug("finalized");

  G_OBJECT_CLASS(chart_parent_class)->finalize(object);
}

void chart_class_init(ChartClass* klass) {
  auto* object_class = G_OBJECT_CLASS(klass);
  auto* widget_class = GTK_WIDGET_CLASS(klass);

  object_class->finalize = finalize;

  widget_class->snapshot = snapshot;
  widget_class->unroot = unroot;

  gtk_widget_class_set_template_from_resource(widget_class, tags::resources::chart_ui);
}

void chart_init(Chart* self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  self->data = new Data();

  self->data->draw_bar_border = true;
  self->data->fill_bars = false;
  self->data->is_visible = true;
  self->data->x_axis_height = 0;
  self->data->y_axis_width = 0;
  self->data->n_x_decimals = 1;
  self->data->n_y_decimals = 1;
  self->data->line_width = 2.0;
  self->data->margin = 0.02;

  self->data->background_color = GdkRGBA{1.0F, 1.0F, 1.0F, 1.0F};
  self->data->color_axis_labels = GdkRGBA{0.0F, 0.0F, 0.0F, 1.0F};
  self->data->gradient_color = GdkRGBA{1.0F, 1.0F, 1.0F, 1.0F};

  self->data->chart_type = ChartType::line;
  self->data->chart_scale = ChartScale::linear;

  self->controller_motion = gtk_event_controller_motion_new();

  self->gesture_single = GTK_GESTURE_SINGLE(gtk_gesture_click_new());

  gtk_gesture_single_set_button(self->gesture_single, 0);

  g_signal_connect(self->gesture_single, "pressed", G_CALLBACK(on_gesture_click_pressed), self);
  g_signal_connect(self->gesture_single, "released", G_CALLBACK(on_gesture_click_released), self);

  g_signal_connect(self->controller_motion, "motion", G_CALLBACK(on_pointer_motion), self);

  g_signal_connect(GTK_WIDGET(self), "hide",
                   G_CALLBACK(+[](GtkWidget* widget, Chart* self) { self->data->is_visible = false; }), self);

  g_signal_connect(GTK_WIDGET(self), "show",
                   G_CALLBACK(+[](GtkWidget* widget, Chart* self) { self->data->is_visible = true; }), self);

  gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(self->gesture_single));
  gtk_widget_add_controller(GTK_WIDGET(self), self->controller_motion);
}

void queue_draw(Chart* self) {
  gtk_widget_queue_draw(GTK_WIDGET(self));
}

auto create() -> Chart* {
  return static_cast<Chart*>(g_object_new(EE_TYPE_CHART, nullptr));
}

}  // namespace ui::chart
