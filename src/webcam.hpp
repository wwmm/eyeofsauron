#pragma once

#include <fcntl.h>
#include <gst/gst.h>
#include <linux/videodev2.h>
#include <sigc++/sigc++.h>
#include <sys/ioctl.h>
#include <cstring>
#include <filesystem>
#include "util.hpp"

class Webcam {
 public:
  Webcam();
  Webcam(const Webcam&) = delete;
  auto operator=(const Webcam&) -> Webcam& = delete;
  Webcam(const Webcam&&) = delete;
  auto operator=(const Webcam&&) -> Webcam& = delete;
  ~Webcam();

  std::string log_tag = "webcam: ";

  void start();

  void pause();

  void stop();

  void get_latency();

  void set_output_resolution(const int& width = 640, const int& height = 480);

  auto get_device_list() -> std::vector<std::string>;

  void set_device(const std::string& name);

  auto get_configured_fps() -> std::string;

  sigc::signal<void(float)> new_framerate;
  sigc::signal<void(uint, uint)> new_frame_size;
  sigc::signal<void(std::vector<guint8>, guint64)> new_frame;
  sigc::signal<void(int)> new_latency;

 private:
  GstElement *pipeline = nullptr, *source = nullptr, *sink = nullptr, *capsfilter_out = nullptr,
             *capsfilter_in = nullptr;

  GstBus* bus = nullptr;

  GstClockTime state_check_timeout = 5 * GST_SECOND;

  struct Devices {
    std::string path, name;

    v4l2_fmtdesc format_description;

    std::vector<v4l2_frmsizeenum> frame_sizes;

    std::vector<std::tuple<int, int, int, int>> resolutions;  // width, height, numerator, denominator

    int width, height, numerator, denominator;
  };

  std::vector<Devices> devices;

  void find_devices();

  void find_devices_frame_sizes();

  void find_devices_frame_intervals();

  void find_best_resolution();
};
