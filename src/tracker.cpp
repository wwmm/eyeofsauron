#include <opencv2/core/hal/interface.h>
#include <qabstractitemmodel.h>
#include <qabstractseries.h>
#include <qbytearray.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qhash.h>
#include <qimage.h>
#include <qlist.h>
#include <qmediaplayer.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qrect.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qurl.h>
#include <qvalueaxis.h>
#include <qvariant.h>
#include <qvideoframeformat.h>
#include <qxyseries.h>
#include <cstddef>
#include <iterator>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <vector>
#include "eyeofsauron_db.h"
#include "frame_source.hpp"
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>
#include <qqml.h>
#include <qsize.h>
#include <KLocalizedString>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPainter>
#include <QVideoFrame>
#include <algorithm>
#include <opencv2/core/types.hpp>
#include "config.h"
#include "tracker.hpp"
#include "util.hpp"

namespace tracker {

int SourceModel::rowCount(const QModelIndex& /*parent*/) const {
  return list.size();
}

QHash<int, QByteArray> SourceModel::roleNames() const {
  return {
      {Roles::SourceType, "sourceType"}, {Roles::Name, "name"}, {Roles::Subtitle, "subtitle"}, {Roles::Icon, "icon"}};
}

QVariant SourceModel::data(const QModelIndex& index, int role) const {
  if (list.empty()) {
    return "";
  }

  const auto it = std::next(list.begin(), index.row());

  switch (role) {
    case Roles::SourceType: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          value = "camera";

          break;
        }
        case VideoFile: {
          value = "video_file";

          break;
        }
      }

      return value;
    }
    case Roles::Name: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          value = dynamic_cast<const CameraSource*>(it->get())->device.description();

          break;
        }
        case VideoFile: {
          value = dynamic_cast<const VideoFileSource*>(it->get())->url.fileName();

          break;
        }
      }

      return value;
    }
    case Roles::Subtitle: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          auto format = dynamic_cast<const CameraSource*>(it->get())->format;

          auto resolution = util::to_string(format.resolution().width()) + "x" +
                            util::to_string(format.resolution().height()) + "  " +
                            util::to_string(format.maxFrameRate()) + " fps";

          value = QString::fromStdString(resolution);

          break;
        }
        case VideoFile: {
          value = "";

          break;
        }
      }

      return value;
    }
    case Roles::Icon: {
      QString name;

      switch (it->get()->source_type) {
        case Camera: {
          name = "camera-web-symbolic";

          break;
        }
        case VideoFile: {
          name = "video-symbolic";

          break;
        }
      }

      return name;
    }
    default:
      return {};
  }
}

auto SourceModel::getList() -> QList<std::shared_ptr<Source>> {
  return list;
}

auto SourceModel::get_source(const int& rowIndex) -> std::shared_ptr<Source> {
  return list[rowIndex];
}

void SourceModel::append(std::shared_ptr<Source> source) {
  int pos = list.empty() ? 0 : list.size() - 1;

  beginInsertRows(QModelIndex(), pos, pos);

  switch (source->source_type) {
    case Camera:
      list.insert(0, source);
      break;
    case VideoFile:
      list.append(source);
      break;
  }

  endInsertRows();

  emit dataChanged(index(0), index(list.size() - 1));
}

void SourceModel::reset() {
  beginResetModel();

  list.clear();

  endResetModel();
}

void SourceModel::removeSource(const int& rowIndex) {
  beginRemoveRows(QModelIndex(), rowIndex, rowIndex);

  list.remove(rowIndex);

  endRemoveRows();

  emit dataChanged(index(0), index(list.size() - 1));
}

Backend::Backend(QObject* parent)
    : QObject(parent),
      camera(std::make_unique<QCamera>()),
      camera_video_sink(std::make_unique<QVideoSink>()),
      capture_session(std::make_unique<QMediaCaptureSession>()),
      media_player(std::make_unique<QMediaPlayer>()),
      media_player_video_sink(std::make_unique<QVideoSink>()) {
  qmlRegisterSingletonInstance<Backend>("EoSTrackerBackend", VERSION_MAJOR, VERSION_MINOR, "EoSTrackerBackend", this);

  qmlRegisterSingletonInstance<SourceModel>("EosTrackerSourceModel", VERSION_MAJOR, VERSION_MINOR,
                                            "EosTrackerSourceModel", &sourceModel);

  connect(this, &Backend::videoSinkChanged, [this]() { draw_offline_image(); });

  connect(camera_video_sink.get(), &QVideoSink::videoFrameChanged, [this](const QVideoFrame& frame) {
    if (!pause_preview) {
      input_video_frame = frame;
      process_frame();
    }
  });

  connect(media_player_video_sink.get(), &QVideoSink::videoFrameChanged, [this](const QVideoFrame& frame) {
    if (!pause_preview) {
      input_video_frame = frame;
      process_frame();
    }
  });

  capture_session->setCamera(camera.get());
  capture_session->setVideoSink(camera_video_sink.get());

  media_player->setVideoSink(media_player_video_sink.get());

  camera->setExposureMode(QCamera::ExposureAction);
  camera->setFocusMode(QCamera::FocusModeAuto);
  camera->setWhiteBalanceMode(QCamera::WhiteBalanceAuto);

  find_best_camera_resolution();
}

Backend::~Backend() {
  camera->stop();
  media_player->stop();
}

void Backend::start() {
  initial_time = 0;
  pause_preview = false;

  switch (current_source_type) {
    case Camera: {
      camera->start();
    } break;
    case VideoFile: {
      media_player->play();
      break;
    }
  }
}

void Backend::pause() {
  pause_preview = true;

  switch (current_source_type) {
    case Camera: {
    } break;
    case VideoFile: {
      media_player->pause();
      break;
    }
  }
}

void Backend::stop() {
  initial_time = 0;

  switch (current_source_type) {
    case Camera: {
      camera->stop();
    } break;
    case VideoFile: {
      media_player->stop();
      break;
    }
  }
}

void Backend::append(const QUrl& videoUrl) {
  if (videoUrl.isLocalFile()) {
    sourceModel.append(std::make_shared<VideoFileSource>(videoUrl));
  }
}

void Backend::selectSource(const int& index) {
  auto source = sourceModel.get_source(index);

  media_player->stop();
  camera->stop();
  trackers.clear();

  switch (source->source_type) {
    case Camera: {
      current_source_type = SourceType::Camera;

      auto device = dynamic_cast<const CameraSource*>(source.get())->device;

      camera->setCameraDevice(dynamic_cast<const CameraSource*>(source.get())->device);
      camera->setCameraFormat(dynamic_cast<const CameraSource*>(source.get())->format);
      camera->start();

      break;
    }
    case VideoFile: {
      current_source_type = SourceType::VideoFile;

      auto url = dynamic_cast<const VideoFileSource*>(source.get())->url;

      media_player->setSource(url);
      media_player->play();

      break;
    }
  }
}

void Backend::find_best_camera_resolution() {
  sourceModel.reset();

  for (const QCameraDevice& cameraDevice : QMediaDevices::videoInputs()) {
    auto formats = cameraDevice.videoFormats();

    if (!formats.empty()) {
      std::ranges::sort(formats, [](QCameraFormat a, QCameraFormat b) {
        if (a.maxFrameRate() > b.maxFrameRate()) {
          return true;
        } else if (a.maxFrameRate() == b.maxFrameRate()) {
          auto area_a = a.resolution().width() * a.resolution().height();
          auto area_b = b.resolution().width() * b.resolution().height();

          return area_a > area_b;
        }

        return false;
      });

      auto resolution = fmt::format("{0}x{1}:{2}", formats.front().resolution().width(),
                                    formats.front().resolution().height(), formats.begin()->maxFrameRate());

      sourceModel.append(std::make_shared<CameraSource>(cameraDevice, formats.front()));

      util::debug(cameraDevice.description().toStdString() + " -> " + resolution);

      auto dev_path = util::v4l2_find_device(cameraDevice.description().toStdString());

      util::v4l2_disable_dynamic_fps(dev_path);
    }
  }
}

void Backend::draw_offline_image() {
  if (_videoSink == nullptr) {
    util::warning("Invalid videoSink pointer!");
  }

  auto video_format = QVideoFrameFormat(QSize(_frameWidth, _frameHeight), QVideoFrameFormat::Format_RGBA8888);

  video_format.setColorRange(QVideoFrameFormat::ColorRange_Full);

  QVideoFrame video_frame(video_format);

  if (!video_frame.isValid() || !video_frame.map(QVideoFrame::WriteOnly)) {
    util::warning("QVideoFrame is not valid or not writable");

    return;
  }

  QImage::Format image_format = QVideoFrameFormat::imageFormatFromPixelFormat(video_frame.pixelFormat());

  if (image_format == QImage::Format_Invalid) {
    util::warning("It is not possible to obtain image format from the pixel format of the videoframe");

    return;
  }

  QImage image(video_frame.bits(0), video_frame.width(), video_frame.height(), image_format);

  QPainter painter(&image);

  painter.drawImage(image.rect(), QImage(":/images/offline.png"));
  painter.drawText(image.rect(), Qt::AlignLeft | Qt::AlignTop, QDateTime::currentDateTime().toString());
  painter.end();

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

void Backend::drawRoiSelection(const bool& state) {
  draw_roi_selection = state;
}

void Backend::createNewRoi(double x, double y, double width, double height) {
  cv::Rect2d roi = {x, y, width, height};  // region of interest that is being created

  auto tracker = cv::legacy::TrackerMOSSE::create();
  // auto tracker = cv::legacy::TrackerMedianFlow::create();

  for (auto& [tracker, roi, initialized, data_tx, data_ty] : trackers) {
    data_tx.clear();
    data_ty.clear();
  }

  trackers.emplace_back(tracker, roi, false, QList<QPointF>(), QList<QPointF>());

  initial_time = 0;
}

void Backend::newRoiSelection(double x, double y, double width, double height) {
  rect_selection.setRect(x, y, width, height);

  if (pause_preview) {
    process_frame();
  }
}

int Backend::removeRoi(double x, double y) {
  initial_time = 0;

  for (size_t n = 0; n < trackers.size(); n++) {
    const auto& [tracker, roi, initialized, data_tx, data_ty] = trackers[n];

    if (x >= roi.x && x < roi.x + roi.width) {
      if (y >= roi.y && y < roi.y + roi.height) {
        std::erase(trackers, trackers[n]);

        return n;
      }
    }
  }

  return -1;
}

void Backend::process_frame() {
  if (_videoSink == nullptr) {
    util::warning("Invalid videoSink pointer!");
  }

  if (!input_video_frame.isValid()) {
    util::warning("QVideoFrame is not valid or not writable");

    return;
  }

  auto input_image = input_video_frame.toImage()
                         .scaled(_frameWidth, _frameHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
                         .convertedTo(QImage::Format_BGR888);

  // creating the output qvideoframe

  auto video_format = QVideoFrameFormat(QSize(_frameWidth, _frameHeight), QVideoFrameFormat::Format_BGRX8888);

  QVideoFrame video_frame(video_format);

  if (!video_frame.isValid() || !video_frame.map(QVideoFrame::WriteOnly)) {
    util::warning("QVideoFrame is not valid or not writable");

    return;
  }

  // creating the qimage that will set the output qvideoframe data

  QImage output_image(video_frame.bits(0), video_frame.width(), video_frame.height(),
                      QVideoFrameFormat::imageFormatFromPixelFormat(video_frame.pixelFormat()));

  if (output_image.isNull()) {
    util::warning("The QImage is null");
    return;
  }

  QPainter painter(&output_image);

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

  painter.drawImage(output_image.rect(), input_image);

  // opencv stuff
  // https://docs.opencv.org/3.4/d3/d63/classcv_1_1Mat.html#a51615ebf17a64c968df0bf49b4de6a3a

  cv::Mat cv_frame(_frameHeight, _frameWidth, CV_8UC3, input_image.bits(), input_image.bytesPerLine());

  initial_time = (initial_time == 0) ? input_video_frame.startTime() : initial_time;

  for (auto& [tracker, roi_n, initialized, data_tx, data_ty] : trackers) {
    if (!initialized) {
      tracker->init(cv_frame, roi_n);

      initialized = true;
    } else {
      tracker->update(cv_frame, roi_n);
    }

    painter.drawRect(QRectF{roi_n.x, roi_n.y, roi_n.width, roi_n.height});

    double xc = roi_n.x + roi_n.width * 0.5;
    double yc = roi_n.y + roi_n.height * 0.5;
    double t = static_cast<double>(input_video_frame.startTime() - initial_time) / 1000000.0;

    // changing the coordinate system origin to the bottom left corner

    yc = _frameHeight - yc;

    data_tx.append(QPointF(t, xc));
    data_ty.append(QPointF(t, yc));

    while (data_tx.size() > db::Main::chartDataPoints()) {
      data_tx.removeFirst();
      data_ty.removeFirst();
    }

    if (!pause_preview) {
      Q_EMIT updateChart();
    }
  }

  if (db::Main::showFps()) {
    painter.drawText(output_image.rect(), Qt::AlignLeft | Qt::AlignBottom,
                     QString::fromStdString(fmt::format(
                         "{0:.0f} fps", 1000000.0 / (input_video_frame.endTime() - input_video_frame.startTime()))));
  }

  if (db::Main::showDateTime()) {
    painter.drawText(output_image.rect(), Qt::AlignLeft | Qt::AlignTop, QDateTime::currentDateTime().toString());
  }

  if (draw_roi_selection) {
    painter.setPen(QColorConstants::Red);
    painter.drawRect(rect_selection);
  }

  painter.end();

  // sending the output qvideoframe to the video sink

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

void Backend::updateSeries(QAbstractSeries* series_x, QAbstractSeries* series_y, const int& index) {
  if (series_x != nullptr && series_y != nullptr) {
    auto xySeries_x = dynamic_cast<QXYSeries*>(series_x);
    auto xySeries_y = dynamic_cast<QXYSeries*>(series_y);

    auto& [tracker, roi_n, initialized, data_tx, data_ty] = trackers[index];

    if (data_tx.empty() || data_ty.empty()) {
      return;
    }

    {
      auto [min_x, max_x] = std::ranges::minmax_element(data_tx, [](QPointF a, QPointF b) { return a.y() < b.y(); });
      auto [min_y, max_y] = std::ranges::minmax_element(data_ty, [](QPointF a, QPointF b) { return a.y() < b.y(); });

      _yAxisMin = std::min(min_x->y(), min_y->y());
      _yAxisMax = std::max(max_x->y(), max_y->y());

      Q_EMIT yAxisMinChanged();
      Q_EMIT yAxisMaxChanged();
    }

    {
      auto [min_t, max_t] = std::ranges::minmax_element(data_tx, [](QPointF a, QPointF b) { return a.x() < b.x(); });

      _xAxisMin = min_t->x();
      _xAxisMax = max_t->x();

      Q_EMIT xAxisMinChanged();
      Q_EMIT xAxisMaxChanged();
    }

    // Use replace instead of clear + append, it's optimized for performance
    xySeries_x->replace(data_tx);
    xySeries_y->replace(data_ty);
  } else {
    util::warning("series_x or series_y is null!");
  }
}

}  // namespace tracker