#include "ui_helpers.hpp"

namespace {

std::locale user_locale;

}  // namespace

namespace ui {

using namespace std::string_literals;

void save_user_locale() {
  try {
    user_locale = std::locale("");
  } catch (...) {
    util::warning("We could not get the user locale in your system! Your locale configuration is broken!");

    util::warning("Falling back to the C locale");
  }
}

auto get_user_locale() -> std::locale {
  return user_locale;
}

auto parse_spinbutton_output(GtkSpinButton* button, const char* unit) -> bool {
  auto* adjustment = gtk_spin_button_get_adjustment(button);
  auto value = gtk_adjustment_get_value(adjustment);
  auto precision = gtk_spin_button_get_digits(button);
  auto str_unit = (unit != nullptr) ? (" "s + unit) : ""s;

  auto text = fmt::format(ui::get_user_locale(), "{0:.{1}Lf}{2}", value, precision, str_unit);

  gtk_editable_set_text(GTK_EDITABLE(button), text.c_str());

  return true;
}

auto parse_spinbutton_input(GtkSpinButton* button, double* new_value) -> int {
  auto min = 0.0;
  auto max = 0.0;

  gtk_spin_button_get_range(button, &min, &max);

  std::istringstream str(gtk_editable_get_text(GTK_EDITABLE(button)));

  str.imbue(ui::get_user_locale());

  auto v = 0.0;

  if (str >> v) {
    *new_value = std::clamp(v, min, max);

    return 1;
  }

  return GTK_INPUT_ERROR;
}

void save_widget_to_png(GtkWidget* widget, const std::string& path) {
  if (widget == nullptr || path.empty()) {
    return;
  }

  auto* paintable = gtk_widget_paintable_new(widget);

  auto* snapshot = gtk_snapshot_new();

  gdk_paintable_snapshot(paintable, snapshot, gdk_paintable_get_intrinsic_width(paintable),
                         gdk_paintable_get_intrinsic_height(paintable));

  auto* node = gtk_snapshot_free_to_node(snapshot);

  if (node != nullptr) {
    auto* renderer = gtk_native_get_renderer(gtk_widget_get_native(widget));

    graphene_rect_t bounds;

    gsk_render_node_get_bounds(node, &bounds);

    auto* texture = gsk_renderer_render_texture(renderer, node, &bounds);

    gdk_texture_save_to_png(texture, path.c_str());

    gsk_render_node_unref(node);
  } else {
    util::warning("failed to save graph to png");
  }

  g_object_unref(paintable);
}

void append_to_string_list(GtkStringList* string_list, const std::string& name) {
  for (guint n = 0U; n < g_list_model_get_n_items(G_LIST_MODEL(string_list)); n++) {
    if (gtk_string_list_get_string(string_list, n) == name) {
      return;
    }
  }

  gtk_string_list_append(string_list, name.c_str());
}

void remove_from_string_list(GtkStringList* string_list, const std::string& name) {
  for (guint n = 0U; n < g_list_model_get_n_items(G_LIST_MODEL(string_list)); n++) {
    if (gtk_string_list_get_string(string_list, n) == name) {
      gtk_string_list_remove(string_list, n);

      return;
    }
  }
}

}  // namespace ui
