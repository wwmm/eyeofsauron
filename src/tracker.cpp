#include "tracker.hpp"

namespace {

enum class VisibleChart { x, y };

int frame_width = 640;
int frame_height = 480;

VisibleChart visible_chart = VisibleChart::x;

guint64 initial_time = 0;

std::unique_ptr<Webcam> webcam_obj;

std::vector<std::tuple<cv::Ptr<cv::legacy::TrackerMOSSE>, cv::Rect2d, bool>> trackers;

}  // namespace

namespace ui::tracker {

using namespace std::string_literals;

struct Data {
 public:
  app::Application* application;
};

struct _Tracker {
  GtkBox parent_instance{};

  GtkDropDown* dropdown_webcam{};

  GtkLabel* fps_label{};

  GtkStringList* device_list{};

  ui::webcam::Webcam* webcam = nullptr;

  ui::chart::Chart *chart_x = nullptr, *chart_y = nullptr;

  Data* data = nullptr;
};

G_DEFINE_TYPE(Tracker, tracker, GTK_TYPE_BOX)

void on_start(Tracker* self, GtkButton* btn) {
  webcam_obj->start();
}

void on_pause(Tracker* self, GtkButton* btn) {
  webcam_obj->pause();
}

void on_stop(Tracker* self, GtkButton* btn) {
  webcam_obj->stop();
}

void on_stack_visible_child_changed(Tracker* self, GParamSpec* pspec, GtkWidget* stack) {
  const auto* name = adw_view_stack_get_visible_child_name(ADW_VIEW_STACK(stack));

  if (g_strcmp0(name, "chart_x") == 0) {
    visible_chart = VisibleChart::x;
  } else if (g_strcmp0(name, "chart_y") == 0) {
    visible_chart = VisibleChart::y;
  }
}

void on_npoints_value_changed(Tracker* self, GtkSpinButton* btn) {
  auto n_points = static_cast<size_t>(gtk_spin_button_get_value(btn));

  ui::chart::set_max_points(self->chart_x, n_points);
  ui::chart::set_max_points(self->chart_y, n_points);
}

void on_new_frame(Tracker* self, const std::vector<guint8>& vector_frame, const guint64& timestamp) {
  ui::webcam::draw_frame(self->webcam, vector_frame, frame_width, frame_height);

  cv::Mat frame = cv::Mat(frame_height, frame_width, CV_8UC3);

  memcpy(frame.data, vector_frame.data(), vector_frame.size() * sizeof(guint8));

  initial_time = (initial_time == 0) ? timestamp : initial_time;

  std::vector<cv::Rect> roi_list;

  for (size_t n = 0; n < trackers.size(); n++) {
    auto& [tracker, roi_n, initialized] = trackers[n];

    if (!initialized) {
      tracker->init(frame, roi_n);

      initialized = true;
    } else {
      tracker->update(frame, roi_n);
    }

    double xc = roi_n.x + roi_n.width * 0.5;
    double yc = roi_n.y + roi_n.height * 0.5;
    double t = static_cast<double>(timestamp - initial_time) / 1000000000.0;

    ui::chart::add_point(self->chart_x, static_cast<int>(n), t, xc);
    ui::chart::update(self->chart_x);

    ui::chart::add_point(self->chart_y, static_cast<int>(n), t, yc);
    ui::chart::update(self->chart_y);

    roi_list.emplace_back(roi_n);
  }

  ui::webcam::update_roi_list(self->webcam, roi_list);

  ui::webcam::queue_draw(self->webcam);
}

void on_save_graph(Tracker* self, GtkButton* button) {
  auto* active_window = gtk_application_get_active_window(GTK_APPLICATION(self->data->application));

  auto* dialog =
      gtk_file_chooser_native_new(_("Save Graph"), active_window, GTK_FILE_CHOOSER_ACTION_SAVE, _("Save"), _("Cancel"));

  auto* filter = gtk_file_filter_new();

  gtk_file_filter_add_pattern(filter, "*.png");
  gtk_file_filter_set_name(filter, _("IMAGE(*.png)"));

  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

  auto* default_dir = g_file_new_for_path(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_dir, nullptr);

  g_object_unref(default_dir);

  g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkNativeDialog* dialog, int response, Tracker* self) {
                     if (response == GTK_RESPONSE_ACCEPT) {
                       auto* chooser = GTK_FILE_CHOOSER(dialog);
                       auto* file = gtk_file_chooser_get_file(chooser);
                       auto* path = g_file_get_path(file);

                       std::filesystem::path output_file{path};

                       if (output_file.extension() != "png") {
                         output_file.replace_filename(output_file.stem().string() + ".png");
                       }

                       util::debug("saving png to path: " + output_file.string());

                       switch (visible_chart) {
                         case VisibleChart::x: {
                           ui::save_widget_to_png(GTK_WIDGET(self->chart_x), output_file.string());

                           break;
                         }
                         case VisibleChart::y: {
                           ui::save_widget_to_png(GTK_WIDGET(self->chart_y), output_file.string());

                           break;
                         }
                       }

                       g_free(path);

                       g_object_unref(file);
                     }

                     g_object_unref(dialog);
                   }),
                   self);

  gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), 1);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

void on_save_table(Tracker* self, GtkButton* button) {
  auto* active_window = gtk_application_get_active_window(GTK_APPLICATION(self->data->application));

  auto* dialog =
      gtk_file_chooser_native_new(_("Save Table"), active_window, GTK_FILE_CHOOSER_ACTION_SAVE, _("Save"), _("Cancel"));

  auto* filter = gtk_file_filter_new();

  gtk_file_filter_add_pattern(filter, "*.tsv");
  gtk_file_filter_set_name(filter, _("Table(*.tsv)"));

  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

  auto* default_dir = g_file_new_for_path(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_dir, nullptr);

  g_object_unref(default_dir);

  g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkNativeDialog* dialog, int response, Tracker* self) {
                     if (response == GTK_RESPONSE_ACCEPT) {
                       auto* chooser = GTK_FILE_CHOOSER(dialog);
                       auto* file = gtk_file_chooser_get_file(chooser);
                       auto* path = g_file_get_path(file);

                       std::filesystem::path output_path{path};

                       auto base_name = output_path.stem();

                       for (size_t n = 0; n < trackers.size(); n++) {
                         std::deque<double> x;
                         std::deque<double> y;

                         switch (visible_chart) {
                           case VisibleChart::x: {
                             std::tie(x, y) = ui::chart::get_curve(self->chart_x, n);
                             break;
                           }
                           case VisibleChart::y: {
                             std::tie(x, y) = ui::chart::get_curve(self->chart_y, n);
                             break;
                           }
                         }

                         if (x.empty()) {
                           break;
                         }

                         output_path.replace_filename(base_name.string() + "_" + std::to_string(n) + ".tsv");

                         util::debug("saving table to path: " + output_path.string());

                         std::ofstream ofs;

                         ofs.open(output_path.string(), std::ofstream::out);

                         for (size_t m = 0; m < x.size(); m++) {
                           ofs << std::scientific << x[m] << "\t" << y[m] << std::endl;
                         }

                         ofs.close();
                       }

                       g_free(path);

                       g_object_unref(file);
                     }

                     g_object_unref(dialog);
                   }),
                   self);

  gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), 1);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

void on_reset_zoom(Tracker* self, GtkButton* button) {
  switch (visible_chart) {
    case VisibleChart::x: {
      ui::chart::reset_zoom(self->chart_x);
      break;
    }
    case VisibleChart::y: {
      ui::chart::reset_zoom(self->chart_y);
      break;
    }
  }
}

void setup(Tracker* self, app::Application* application) {
  self->data->application = application;
}

void dispose(GObject* object) {
  // auto* self = WW_TRACKER(object);

  G_OBJECT_CLASS(tracker_parent_class)->dispose(object);
}

void finalize(GObject* object) {
  auto* self = WW_TRACKER(object);

  delete self->data;

  G_OBJECT_CLASS(tracker_parent_class)->finalize(object);
}

void tracker_class_init(TrackerClass* klass) {
  auto* object_class = G_OBJECT_CLASS(klass);
  auto* widget_class = GTK_WIDGET_CLASS(klass);

  object_class->dispose = dispose;
  object_class->finalize = finalize;

  // A call to this function registers the custom type. Once registerred it can be used in the xml file

  chart::chart_get_type();
  webcam::webcam_get_type();

  gtk_widget_class_set_template_from_resource(widget_class, tags::resources::tracker_ui);

  gtk_widget_class_bind_template_child(widget_class, Tracker, webcam);
  gtk_widget_class_bind_template_child(widget_class, Tracker, chart_x);
  gtk_widget_class_bind_template_child(widget_class, Tracker, chart_y);
  gtk_widget_class_bind_template_child(widget_class, Tracker, dropdown_webcam);
  gtk_widget_class_bind_template_child(widget_class, Tracker, device_list);
  gtk_widget_class_bind_template_child(widget_class, Tracker, fps_label);

  gtk_widget_class_bind_template_callback(widget_class, on_start);
  gtk_widget_class_bind_template_callback(widget_class, on_pause);
  gtk_widget_class_bind_template_callback(widget_class, on_stop);
  gtk_widget_class_bind_template_callback(widget_class, on_stack_visible_child_changed);
  gtk_widget_class_bind_template_callback(widget_class, on_npoints_value_changed);
  gtk_widget_class_bind_template_callback(widget_class, on_save_graph);
  gtk_widget_class_bind_template_callback(widget_class, on_save_table);
  gtk_widget_class_bind_template_callback(widget_class, on_reset_zoom);
}

void tracker_init(Tracker* self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  self->data = new Data();

  webcam_obj = std::make_unique<Webcam>();

  webcam_obj->set_output_resolution(frame_width, frame_height);

  webcam_obj->new_frame.connect(
      [=](const std::vector<guint8>& frame, const guint64& timestamp) { on_new_frame(self, frame, timestamp); });

  ui::webcam::new_roi.connect([=](cv::Rect new_roi) {
    ui::chart::add_curve(self->chart_x);
    ui::chart::add_curve(self->chart_y);

    auto tracker = cv::legacy::TrackerMOSSE::create();

    trackers.emplace_back(tracker, new_roi, false);
  });

  ui::webcam::remove_roi.connect([=](const int& x, const int& y) {
    for (size_t n = 0; n < trackers.size(); n++) {
      const auto& [tracker, roi, initialized] = trackers[n];

      if (x > roi.x && x < roi.x + roi.width) {
        if (y > roi.y && y < roi.y + roi.height) {
          trackers.erase(std::remove(trackers.begin(), trackers.end(), trackers[n]), trackers.end());

          ui::chart::remove_curve(self->chart_x, static_cast<int>(n));
          ui::chart::remove_curve(self->chart_y, static_cast<int>(n));

          ui::chart::update(self->chart_x);
          ui::chart::update(self->chart_y);

          return;
        }
      }
    }
  });

  auto device_list = webcam_obj->get_device_list();

  for (const auto& name : device_list) {
    ui::append_to_string_list(self->device_list, name);
  }

  gtk_label_set_text(self->fps_label, webcam_obj->get_configured_fps().c_str());

  g_signal_connect(self->dropdown_webcam, "notify::selected-item",
                   G_CALLBACK(+[](GtkDropDown* dropdown, GParamSpec* pspec, Tracker* self) {
                     if (auto selected_item = gtk_drop_down_get_selected_item(dropdown); selected_item != nullptr) {
                       auto name = gtk_string_object_get_string(GTK_STRING_OBJECT(selected_item));

                       webcam_obj->set_device(name);

                       gtk_label_set_text(self->fps_label, webcam_obj->get_configured_fps().c_str());
                     }
                   }),
                   self);
}

auto create() -> Tracker* {
  return static_cast<Tracker*>(g_object_new(WW_TYPE_TRACKER, nullptr));
}

}  // namespace ui::tracker
