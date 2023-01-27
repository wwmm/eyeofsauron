#pragma once

#include <adwaita.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <sigc++/sigc++.h>
#include <mutex>
#include <opencv2/core/types.hpp>
#include "colors.hpp"
#include "tags_app.hpp"
#include "tags_resources.hpp"
#include "util.hpp"

namespace ui::webcam {

G_BEGIN_DECLS

#define WW_TYPE_WEBCAM (webcam_get_type())

G_DECLARE_FINAL_TYPE(Webcam, webcam, WW, WEBCAM, GtkWidget)

G_END_DECLS

auto create() -> Webcam*;

void draw_frame(Webcam* self, const std::vector<guint8>& frame, const int& width, const int& height);

void update_roi_list(Webcam* self, const std::vector<cv::Rect>& list);

void queue_draw(Webcam* self);

extern sigc::signal<void(cv::Rect)> new_roi;
extern sigc::signal<void(int, int)> remove_roi;

}  // namespace ui::webcam
