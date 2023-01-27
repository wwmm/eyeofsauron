#include "webcam_ui.hpp"

namespace ui::webcam {

using namespace std::string_literals;

sigc::signal<void(cv::Rect)> new_roi;
sigc::signal<void(int, int)> remove_roi;

bool primary_mouse_button_pressed = false;
bool secondary_mouse_button_pressed = false;

float radius = 2.0F;

double line_width = 2.0;

cv::Rect roi;  // region of interest that is being created

GdkRGBA new_roi_color = GdkRGBA{1.0F, 0.0F, 0.0F, 1.0F};

GdkPixbuf* camera_pixbuf = nullptr;

std::vector<guint8> current_frame;

std::vector<cv::Rect> roi_list;

std::mutex data_mutex;

struct _Webcam {
  GtkBox parent_instance;

  GtkGestureSingle* gesture_single;

  GtkEventController* controller_motion;
};

G_DEFINE_TYPE(Webcam, webcam, GTK_TYPE_WIDGET)

void queue_draw(Webcam* self) {
  gtk_widget_queue_draw(GTK_WIDGET(self));
}

void draw_frame(Webcam* self, const std::vector<guint8>& frame, const int& width, const int& height) {
  int rowstride = 3 * width;

  std::scoped_lock<std::mutex> lock(data_mutex);

  current_frame.resize(frame.size());

  std::copy(frame.begin(), frame.end(), current_frame.begin());

  g_object_unref(camera_pixbuf);

  camera_pixbuf = gdk_pixbuf_new_from_data(current_frame.data(), GDK_COLORSPACE_RGB, 0, 8, width, height, rowstride,
                                           nullptr, nullptr);
}

void update_roi_list(Webcam* self, const std::vector<cv::Rect>& list) {
  std::scoped_lock<std::mutex> lock(data_mutex);

  roi_list.resize(list.size());

  std::copy(list.begin(), list.end(), roi_list.begin());
}

void on_gesture_click_pressed(GtkGestureClick* gesture_object, gint n_press, gdouble x, gdouble y, gpointer user_data) {
  auto* self = WW_WEBCAM(user_data);

  switch (gtk_gesture_single_get_current_button(self->gesture_single)) {
    case GDK_BUTTON_PRIMARY: {
      primary_mouse_button_pressed = true;

      roi.x = static_cast<int>(x);
      roi.y = static_cast<int>(y);
      roi.width = 0;
      roi.height = 0;

      break;
    }
    case GDK_BUTTON_SECONDARY: {
      secondary_mouse_button_pressed = true;

      util::idle_add([=]() { remove_roi.emit(static_cast<int>(x), static_cast<int>(y)); });

      queue_draw(self);

      break;
    }
  }
}

void on_gesture_click_released(GtkGestureClick* gesture_object,
                               gint n_press,
                               gdouble x,
                               gdouble y,
                               gpointer user_data) {
  auto* self = WW_WEBCAM(user_data);

  switch (gtk_gesture_single_get_current_button(self->gesture_single)) {
    case GDK_BUTTON_PRIMARY: {
      primary_mouse_button_pressed = false;

      roi.width = static_cast<int>(x) - roi.x;
      roi.height = static_cast<int>(y) - roi.y;

      if (std::abs(roi.width) == 0 || std::abs(roi.height) == 0) {
        return;
      }

      if (roi.width < 0) {
        roi.x = roi.width + roi.x;

        roi.width *= -1;
      }

      if (roi.height < 0) {
        roi.y = roi.height + roi.y;

        roi.height *= -1;
      }

      util::debug(
          fmt::format("new roi: x = {0}, y = {1}, width = {2}, height = {3}", roi.x, roi.y, roi.width, roi.height));

      util::idle_add([=]() { new_roi.emit(roi); });

      return;
    }
    case GDK_BUTTON_SECONDARY: {
      secondary_mouse_button_pressed = false;

      return;
    }
  }
}

void on_pointer_motion(GtkEventControllerMotion* controller, double x, double y, Webcam* self) {
  if (primary_mouse_button_pressed) {
    roi.width = static_cast<int>(x) - roi.x;
    roi.height = static_cast<int>(y) - roi.y;

    queue_draw(self);
  }
}

void snapshot(GtkWidget* widget, GtkSnapshot* snapshot) {
  // auto* self = WW_WEBCAM(widget);

  auto width = gtk_widget_get_width(widget);
  auto height = gtk_widget_get_height(widget);

  auto widget_rectangle = GRAPHENE_RECT_INIT(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height));

  auto* ctx = gtk_snapshot_append_cairo(snapshot, &widget_rectangle);

  std::scoped_lock<std::mutex> lock(data_mutex);

  gdk_cairo_set_source_pixbuf(ctx, camera_pixbuf, 0.0, 0.0);

  cairo_paint(ctx);

  cairo_destroy(ctx);

  std::array<float, 4> border_width = {static_cast<float>(line_width), static_cast<float>(line_width),
                                       static_cast<float>(line_width), static_cast<float>(line_width)};

  // drawing the new roi

  if (primary_mouse_button_pressed) {
    auto rectangle = GRAPHENE_RECT_INIT(static_cast<float>(roi.x), static_cast<float>(roi.y),
                                        static_cast<float>(roi.width), static_cast<float>(roi.height));

    GskRoundedRect outline;

    auto border_color = std::to_array({new_roi_color, new_roi_color, new_roi_color, new_roi_color});

    gsk_rounded_rect_init_from_rect(&outline, &rectangle, radius);

    gtk_snapshot_append_border(snapshot, &outline, border_width.data(), border_color.data());
  }

  // drawing the roi list

  for (size_t n = 0; n < roi_list.size(); n++) {
    if (n < ui::colors::list.size()) {
      auto r = roi_list[n];

      auto rectangle = GRAPHENE_RECT_INIT(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.width),
                                          static_cast<float>(r.height));

      GskRoundedRect outline;

      gsk_rounded_rect_init_from_rect(&outline, &rectangle, radius);

      auto border_color =
          std::to_array({ui::colors::list[n], ui::colors::list[n], ui::colors::list[n], ui::colors::list[n]});

      gtk_snapshot_append_border(snapshot, &outline, border_width.data(), border_color.data());
    }
  }
}

void unroot(GtkWidget* widget) {
  // auto* self = WW_WEBCAM(widget);

  GTK_WIDGET_CLASS(webcam_parent_class)->unmap(widget);
}

void finalize(GObject* object) {
  // auto* self = WW_WEBCAM(object);

  g_object_unref(camera_pixbuf);

  util::debug("finalized");

  G_OBJECT_CLASS(webcam_parent_class)->finalize(object);
}

void webcam_class_init(WebcamClass* klass) {
  auto* object_class = G_OBJECT_CLASS(klass);
  auto* widget_class = GTK_WIDGET_CLASS(klass);

  object_class->finalize = finalize;

  widget_class->snapshot = snapshot;
  widget_class->unroot = unroot;

  gtk_widget_class_set_template_from_resource(widget_class, tags::resources::webcam_ui);
}

void webcam_init(Webcam* self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  camera_pixbuf = gdk_pixbuf_new_from_resource(tags::resources::offline_png, nullptr);

  self->controller_motion = gtk_event_controller_motion_new();

  self->gesture_single = GTK_GESTURE_SINGLE(gtk_gesture_click_new());

  gtk_gesture_single_set_button(self->gesture_single, 0);

  g_signal_connect(self->gesture_single, "pressed", G_CALLBACK(on_gesture_click_pressed), self);
  g_signal_connect(self->gesture_single, "released", G_CALLBACK(on_gesture_click_released), self);

  g_signal_connect(self->controller_motion, "motion", G_CALLBACK(on_pointer_motion), self);

  gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(self->gesture_single));
  gtk_widget_add_controller(GTK_WIDGET(self), self->controller_motion);
}

auto create() -> Webcam* {
  return static_cast<Webcam*>(g_object_new(WW_TYPE_WEBCAM, nullptr));
}

}  // namespace ui::webcam
