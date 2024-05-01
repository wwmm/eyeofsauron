#include "util.hpp"
#include <bits/types/struct_sched_param.h>
#include <qdebug.h>
#include <qlogging.h>
#include <sched.h>
#include <sys/types.h>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <ext/string_conversions.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

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

}  // namespace util
