#pragma once

#include <adwaita.h>
#include <sigc++/sigc++.h>
#include <string>
#include "config.h"
#include "tags_app.hpp"
#include "tags_resources.hpp"
#include "tracker.hpp"
#include "ui_helpers.hpp"
#include "util.hpp"

namespace ui::application_window {

G_BEGIN_DECLS

#define EOS_TYPE_APPLICATION_WINDOW (application_window_get_type())

G_DECLARE_FINAL_TYPE(ApplicationWindow, application_window, WW, APP_WINDOW, AdwApplicationWindow)

G_END_DECLS

auto create(GApplication* gapp) -> ApplicationWindow*;

}  // namespace ui::application_window
