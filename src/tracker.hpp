#pragma once

#include <adwaita.h>
#include <glib/gi18n.h>
#include <fstream>
#include <memory>
#include <opencv2/core/types.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>
#include <tuple>
#include "application.hpp"
#include "chart.hpp"
#include "tags_resources.hpp"
#include "webcam.hpp"
#include "webcam_ui.hpp"

namespace ui::tracker {

G_BEGIN_DECLS

#define WW_TYPE_TRACKER (tracker_get_type())

G_DECLARE_FINAL_TYPE(Tracker, tracker, WW, TRACKER, GtkBox)

G_END_DECLS

auto create() -> Tracker*;

void setup(Tracker* self, app::Application* application);

}  // namespace ui::tracker
