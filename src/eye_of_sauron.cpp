#include <glib-unix.h>
#include "application.hpp"
#include "config.h"
#include "tags_app.hpp"

auto sigterm(void* data) -> bool {
  auto* app = G_APPLICATION(data);

  app::hide_all_windows(app);

  g_application_quit(app);

  return G_SOURCE_REMOVE;
}

auto main(int argc, char* argv[]) -> int {
  util::debug("Eye Of Sauron version: " + std::string(VERSION));

  try {
    // Init internationalization support before anything else

    auto* bindtext_output = bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    if (bindtext_output != nullptr) {
      util::debug("locale directory: " + std::string(bindtext_output));
    } else if (errno == ENOMEM) {
      util::warning("bindtextdomain: Not enough memory available!");

      return errno;
    }

    g_setenv("PULSE_PROP_application.id", tags::app::id, 1);
    g_setenv("PULSE_PROP_application.icon_name", APP_NAME, 1);

    auto* app = app::application_new();

    g_unix_signal_add(2, G_SOURCE_FUNC(sigterm), app);

    auto status = g_application_run(app, argc, argv);

    g_object_unref(app);

    util::debug("Exitting the main function with status: " + util::to_string(status, ""));

    return status;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;

    return EXIT_FAILURE;
  }
}
