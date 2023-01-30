#include "webcam.hpp"

namespace {

void on_message_error(const GstBus* gst_bus, GstMessage* message, Webcam* wc) {
  GError* err = nullptr;
  gchar* debug = nullptr;

  gst_message_parse_error(message, &err, &debug);

  util::critical(wc->log_tag + err->message);
  util::debug(wc->log_tag + debug);

  wc->stop();

  g_error_free(err);
  g_free(debug);
}

void on_message_state_changed(const GstBus* gst_bus, GstMessage* message, Webcam* wc) {
  if (std::strcmp(GST_OBJECT_NAME(message->src), "pipeline") == 0) {
    GstState old_state = GST_STATE_NULL;
    GstState new_state = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;

    gst_message_parse_state_changed(message, &old_state, &new_state, &pending);

    util::debug(wc->log_tag + gst_element_state_get_name(old_state) + " -> " + gst_element_state_get_name(new_state) +
                " -> " + gst_element_state_get_name(pending));
  }
}

void on_message_latency(const GstBus* gst_bus, GstMessage* message, Webcam* wc) {
  wc->get_latency();
}

void on_output_type_changed(GstElement* typefind, guint probability, GstCaps* caps, Webcam* wc) {
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  int framerate_numerator = 0;
  int framerate_denomimnator = 0;
  int width = 0;
  int height = 0;

  gst_structure_get_fraction(structure, "framerate", &framerate_numerator, &framerate_denomimnator);
  gst_structure_get_int(structure, "width", &width);
  gst_structure_get_int(structure, "height", &height);

  util::idle_add([=]() { wc->new_frame_size.emit(width, height); });

  util::debug(wc->log_tag + "frame rate: " + std::to_string(framerate_numerator) + "/" +
              std::to_string(framerate_denomimnator));
  util::debug(wc->log_tag + "width: " + std::to_string(width));
  util::debug(wc->log_tag + "height: " + std::to_string(height));
}

auto on_probe_buffer(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) -> GstPadProbeReturn {
  auto* wc = static_cast<Webcam*>(user_data);

  auto* buffer = gst_pad_probe_info_get_buffer(info);

  gsize buffer_size = gst_buffer_get_size(buffer);

  auto pts = GST_BUFFER_PTS(buffer);  // presentation timestamp in nanoseconds

  std::vector<guint8> data(buffer_size);

  gst_buffer_extract(buffer, 0, data.data(), buffer_size);

  util::idle_add([=]() { wc->new_frame.emit(data, pts); });

  return GST_PAD_PROBE_OK;
}

}  // namespace

Webcam::Webcam() {
  gst_init(nullptr, nullptr);

  uint n_cpu_cores = std::thread::hardware_concurrency();

  util::debug("videoconvert plugin will use " + std::to_string(n_cpu_cores) + " cpu cores");

  pipeline = gst_pipeline_new("pipeline");

  bus = gst_element_get_bus(pipeline);

  gst_bus_enable_sync_message_emission(bus);
  gst_bus_add_signal_watch(bus);

  // bus callbacks

  g_signal_connect(bus, "message::error", G_CALLBACK(on_message_error), this);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_message_state_changed), this);
  g_signal_connect(bus, "message::latency", G_CALLBACK(on_message_latency), this);
  //   g_signal_connect(bus, "message::element", G_CALLBACK(on_message_element), this);

  //   gst_registry_scan_path(gst_registry_get(), PLUGINS_INSTALL_DIR);

  source = gst_element_factory_make("v4l2src", "video_src");
  auto* capsfilter_in = gst_element_factory_make("capsfilter", nullptr);
  auto* jpegdec = gst_element_factory_make("jpegdec", nullptr);
  auto* videoconvert = gst_element_factory_make("videoconvert", nullptr);
  capsfilter_out = gst_element_factory_make("capsfilter", nullptr);
  auto* out_type = gst_element_factory_make("typefind", nullptr);
  sink = gst_element_factory_make("fakesink", "video_sink");

  g_object_set(source, "do-timestamp", 1, nullptr);
  g_object_set(videoconvert, "n-threads", n_cpu_cores, nullptr);

  // disabling variable exposure because it leads to variable framerate
  auto* controls = gst_structure_new_empty("extra_controls");
  GValue auto_exposure = G_VALUE_INIT;
  g_value_init(&auto_exposure, G_TYPE_INT);
  g_value_set_int(&auto_exposure, 0);

  gst_structure_take_value(controls, "exposure_auto_priority", &auto_exposure);

  g_object_set(source, "extra-controls", controls, nullptr);

  util::debug(gst_structure_to_string(controls));

  auto* caps = gst_caps_from_string("image/jpeg,framerate=30/1");

  g_object_set(capsfilter_in, "caps", caps, nullptr);

  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter_in, jpegdec, videoconvert, capsfilter_out, out_type, sink,
                   nullptr);

  gst_element_link_many(source, capsfilter_in, jpegdec, videoconvert, capsfilter_out, out_type, sink, nullptr);

  g_signal_connect(out_type, "have-type", G_CALLBACK(on_output_type_changed), this);

  auto* sinkpad = gst_element_get_static_pad(sink, "sink");

  gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, on_probe_buffer, this, nullptr);

  g_object_unref(sinkpad);
}

Webcam::~Webcam() {
  stop();
  gst_object_unref(bus);
  gst_object_unref(pipeline);
}

void Webcam::find_devices() {}

void Webcam::start() {
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void Webcam::pause() {
  gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

void Webcam::stop() {
  gst_element_set_state(pipeline, GST_STATE_NULL);
}

void Webcam::set_resolution(const int& width, const int& height) {
  auto* caps = gst_caps_from_string(
      std::string("video/x-raw,format=RGB,width=" + std::to_string(width) + ",height=" + std::to_string(height))
          .c_str());

  g_object_set(capsfilter_out, "caps", caps, nullptr);

  gst_caps_unref(caps);
}

void Webcam::get_latency() {
  GstQuery* q = gst_query_new_latency();

  if (gst_element_query(pipeline, q) != 0) {
    gboolean live = 0;
    GstClockTime min = 0;
    GstClockTime max = 0;

    gst_query_parse_latency(q, &live, &min, &max);

    int latency = GST_TIME_AS_MSECONDS(min);

    util::debug(log_tag + "total latency: " + std::to_string(latency) + " ms");

    util::idle_add([=, this]() { new_latency.emit(latency); });
  }

  gst_query_unref(q);
}