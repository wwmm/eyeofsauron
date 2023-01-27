#ifndef MICROPHONE_HPP
#define MICROPHONE_HPP

#include <gst/gst.h>
#include <sigc++/sigc++.h>
#include <iostream>

class Microphone {
 public:
  Microphone();
  Microphone(const Microphone&) = delete;
  auto operator=(const Microphone&) -> Microphone& = delete;
  Microphone(const Microphone&&) = delete;
  auto operator=(const Microphone &&) -> Microphone& = delete;
  ~Microphone();

  void start();
  void pause();
  void stop();
  void get_latency();

  sigc::signal<void, std::vector<float>> new_spectrum;
  sigc::signal<void, std::vector<guint16>, guint64> new_frame;
  sigc::signal<void, int> new_latency;

  std::string log_tag = "Microphone: ";

  uint sampling_rate = 48000U;

  uint min_spectrum_freq = 20U;     // Hz
  uint max_spectrum_freq = 20000U;  // Hz
  int spectrum_threshold = -120;    // dB
  uint spectrum_nbands = 3600U, spectrum_nfreqs = 0U;
  uint spectrum_start_index = 0U;

  std::vector<float> spectrum_freqs, spectrum_mag;

 private:
  GstElement *pipeline = nullptr, *source = nullptr, *sink = nullptr, *capsfilter_out = nullptr, *spectrum = nullptr;

  GstBus* bus = nullptr;

  GstClockTime state_check_timeout = 5 * GST_SECOND;
};

#endif