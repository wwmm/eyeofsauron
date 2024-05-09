#include "util.hpp"
#include <bits/types/struct_sched_param.h>
#include <fcntl.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <qdebug.h>
#include <qlogging.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstring>
#include <ext/string_conversions.h>
#include <filesystem>
#include <string>

namespace util {

auto prepare_debug_message(const std::string& message, source_location location) -> std::string {
  auto file_path = std::filesystem::path{location.file_name()};

  std::string msg = file_path.filename().string() + ":" + to_string(location.line()) + "\t" + message;

  return msg;
}

void debug(const std::string& s, source_location location) {
  qDebug().noquote() << prepare_debug_message(s, location);
}

void fatal(const std::string& s, source_location location) {
  qFatal().noquote() << prepare_debug_message(s, location);
}

void critical(const std::string& s, source_location location) {
  qCritical().noquote() << prepare_debug_message(s, location);
}

void warning(const std::string& s, source_location location) {
  qWarning().noquote() << prepare_debug_message(s, location);
}

void info(const std::string& s, source_location location) {
  qInfo().noquote() << prepare_debug_message(s, location);
}

auto v4l2_find_device(const std::string& description) -> std::string {
  for (const auto& entry : std::filesystem::directory_iterator("/dev/")) {
    auto child_path = std::filesystem::path(entry);

    if (std::filesystem::is_character_file(child_path)) {
      if (child_path.stem().string().starts_with("video")) {
        auto device = child_path.string();

        auto fd = open(device.c_str(), O_RDONLY);

        if (fd < 0) {
          util::warning("could not open device: " + device);

          continue;
        }

        std::string name;

        v4l2_capability caps = {};

        if (ioctl(fd, VIDIOC_QUERYCAP, &caps) == 0) {
          if (reinterpret_cast<char*>(caps.card) == description) {
            close(fd);
            return device;
          }
        }

        close(fd);
      }
    }
  }

  return "";
}

void v4l2_disable_dynamic_fps(const std::string& device_path) {
  auto fd = open(device_path.c_str(), O_RDWR);

  if (fd < 0) {
    util::warning("could not open device: " + device_path);

    return;
  }

  v4l2_queryctrl queryctrl{};

  memset(&queryctrl, 0, sizeof(queryctrl));

  queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

  while (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
    // util::warning(reinterpret_cast<char*>(queryctrl.name));

    if (queryctrl.id == V4L2_CID_EXPOSURE_AUTO_PRIORITY) {
      v4l2_control control{};

      memset(&control, 0, sizeof(control));

      control.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;

      control.value = 0;

      if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
        util::warning("v4l2: failed to set EXPOSURE_AUTO_PRIORITY");

        close(fd);

        return;
      }
    }

    queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
  }

  close(fd);
}

}  // namespace util
