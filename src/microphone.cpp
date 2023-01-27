#include "microphone.hpp"
#include <cstring>
#include <string>
#include "glibmm/main.h"
#include "util.hpp"

namespace {

void on_message_error(const GstBus* gst_bus, GstMessage* message, Microphone* m) {
  GError* err = nullptr;
  gchar* debug = nullptr;

  gst_message_parse_error(message, &err, &debug);

  util::critical(m->log_tag + err->message);
  util::debug(m->log_tag + debug);

  m->stop();

  g_error_free(err);
  g_free(debug);
}

void on_message_state_changed(const GstBus* gst_bus, GstMessage* message, Microphone* m) {
  if (std::strcmp(GST_OBJECT_NAME(message->src), "pipeline") == 0) {
    GstState old_state = GST_STATE_NULL;
    GstState new_state = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;

    gst_message_parse_state_changed(message, &old_state, &new_state, &pending);

    util::debug(m->log_tag + gst_element_state_get_name(old_state) + " -> " + gst_element_state_get_name(new_state) +
                " -> " + gst_element_state_get_name(pending));
  }
}

void on_message_latency(const GstBus* gst_bus, GstMessage* message, Microphone* m) {
  m->get_latency();
}

void on_output_type_changed(GstElement* typefind, guint probability, GstCaps* caps, Microphone* m) {
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  util::debug(m->log_tag + std::string(gst_structure_to_string(structure)));
}

auto on_probe_buffer(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) -> GstPadProbeReturn {
  auto* m = static_cast<Microphone*>(user_data);
  auto* buffer = gst_pad_probe_info_get_buffer(info);
  gsize buffer_size = gst_buffer_get_size(buffer);
  auto pts = GST_BUFFER_PTS(buffer);  // presentation timestamp in nanoseconds

  std::vector<guint16> data(buffer_size);

  gst_buffer_extract(buffer, 0, data.data(), buffer_size);

  Glib::signal_idle().connect_once([=] { m->new_frame.emit(data, pts); });

  return GST_PAD_PROBE_OK;
}

void on_message_element(const GstBus* gst_bus, GstMessage* message, Microphone* m) {
  if (std::strcmp(GST_OBJECT_NAME(message->src), "spectrum") == 0) {
    const GstStructure* s = gst_message_get_structure(message);

    const GValue* magnitudes = nullptr;

    magnitudes = gst_structure_get_value(s, "magnitude");

    for (uint n = 0U; n < m->spectrum_freqs.size(); n++) {
      m->spectrum_mag[n] = g_value_get_float(gst_value_list_get_value(magnitudes, n + m->spectrum_start_index));
    }

    auto min_mag = m->spectrum_threshold;
    auto max_mag = *std::max_element(m->spectrum_mag.begin(), m->spectrum_mag.end());

    if (max_mag > min_mag) {
      for (float& v : m->spectrum_mag) {
        if (min_mag < v) {
          v = (min_mag - v) / min_mag;
        } else {
          v = 0.0F;
        }
      }

      Glib::signal_idle().connect_once([=] { m->new_spectrum.emit(m->spectrum_mag); });
    }
  }
}

}  // namespace

Microphone::Microphone() {
  gst_init(nullptr, nullptr);

  pipeline = gst_pipeline_new("pipeline");

  bus = gst_element_get_bus(pipeline);

  gst_bus_enable_sync_message_emission(bus);
  gst_bus_add_signal_watch(bus);

  // bus callbacks

  g_signal_connect(bus, "message::error", G_CALLBACK(on_message_error), this);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_message_state_changed), this);
  g_signal_connect(bus, "message::latency", G_CALLBACK(on_message_latency), this);
  g_signal_connect(bus, "message::element", G_CALLBACK(on_message_element), this);

  //   gst_registry_scan_path(gst_registry_get(), PLUGINS_INSTALL_DIR);

  source = gst_element_factory_make("pulsesrc", "audio_src");
  auto* queue = gst_element_factory_make("queue", nullptr);
  auto* audioconvert = gst_element_factory_make("audioconvert", nullptr);
  capsfilter_out = gst_element_factory_make("capsfilter", nullptr);
  auto* out_type = gst_element_factory_make("typefind", nullptr);
  spectrum = gst_element_factory_make("spectrum", "spectrum");
  sink = gst_element_factory_make("fakesink", "mic_sink");

  g_object_set(source, "do-timestamp", 1, nullptr);
  g_object_set(source, "client-name", "EyeOfSauron::SoundWave", nullptr);
  g_object_set(queue, "silent", 1, nullptr);
  g_object_set(spectrum, "bands", spectrum_nbands, nullptr);
  g_object_set(spectrum, "threshold", spectrum_threshold, nullptr);

  auto* caps = gst_caps_from_string(
      std::string("audio/x-raw,format=S16LE,channels=1,rate=" + std::to_string(sampling_rate)).c_str());

  g_object_set(capsfilter_out, "caps", caps, nullptr);

  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline), source, queue, audioconvert, capsfilter_out, out_type, spectrum, sink, nullptr);

  gst_element_link_many(source, queue, audioconvert, capsfilter_out, out_type, spectrum, sink, nullptr);

  g_signal_connect(out_type, "have-type", G_CALLBACK(on_output_type_changed), this);

  auto* sinkpad = gst_element_get_static_pad(sink, "sink");

  gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, on_probe_buffer, this, nullptr);

  g_object_unref(sinkpad);
}

Microphone::~Microphone() {
  stop();
  gst_object_unref(bus);
  gst_object_unref(pipeline);
}

void Microphone::start() {
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void Microphone::pause() {
  gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

void Microphone::stop() {
  gst_element_set_state(pipeline, GST_STATE_NULL);
}

void Microphone::get_latency() {
  GstQuery* q = gst_query_new_latency();

  if (gst_element_query(pipeline, q) != 0) {
    gboolean live = 0;
    GstClockTime min = 0;
    GstClockTime max = 0;

    gst_query_parse_latency(q, &live, &min, &max);

    int latency = GST_TIME_AS_MSECONDS(min);

    util::debug(log_tag + "total latency: " + std::to_string(latency) + " ms");

    Glib::signal_idle().connect_once([=] { new_latency.emit(latency); });
  }

  gst_query_unref(q);
}