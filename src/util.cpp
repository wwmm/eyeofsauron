#include "util.hpp"
#include <cmath>
#include <utility>

namespace util {

auto prepare_debug_message(const std::string& message, source_location location) -> std::string {
  auto file_path = std::filesystem::path{location.file_name()};

  std::string msg = "\t" + file_path.filename().string() + ":" + to_string(location.line()) + "\t" + message;

  return msg;
}

void debug(const std::string& s, source_location location) {
  g_debug(prepare_debug_message(s, location).c_str(), "%s");
}

void error(const std::string& s, source_location location) {
  g_error(prepare_debug_message(s, location).c_str(), "%s");
}

void critical(const std::string& s, source_location location) {
  g_critical(prepare_debug_message(s, location).c_str(), "%s");
}

void warning(const std::string& s, source_location location) {
  g_warning(prepare_debug_message(s, location).c_str(), "%s");
}

void info(const std::string& s, source_location location) {
  g_info(prepare_debug_message(s, location).c_str(), "%s");
}

void reset_all_keys_except(GSettings* settings, const std::vector<std::string>& blocklist) {
  GSettingsSchema* schema = nullptr;
  gchar** keys = nullptr;

  g_object_get(settings, "settings-schema", &schema, nullptr);

  keys = g_settings_schema_list_keys(schema);

  for (int i = 0; keys[i] != nullptr; i++) {
    if (std::ranges::find(blocklist, keys[i]) == blocklist.end()) {
      g_settings_reset(settings, keys[i]);
    }
  }

  g_settings_schema_unref(schema);
  g_strfreev(keys);
}

void idle_add(std::function<void()> cb) {
  struct Data {
    std::function<void()> cb;
  };

  auto* d = new Data();

  d->cb = std::move(cb);

  g_idle_add((GSourceFunc) +
                 [](Data* d) {
                   if (d == nullptr) {
                     return G_SOURCE_REMOVE;
                   }

                   if (d->cb == nullptr) {
                     return G_SOURCE_REMOVE;
                   }

                   d->cb();

                   delete d;

                   return G_SOURCE_REMOVE;
                 },
             d);
}

}  // namespace util
