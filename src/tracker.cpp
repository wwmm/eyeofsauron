#include "tracker.hpp"
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
#include <qqml.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>
#include <qvalueaxis.h>
#include <qvariant.h>
#include <qvideoframeformat.h>
#include <qxyseries.h>
#include <KLocalizedString>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPainter>
#include <QVideoFrame>
#include <algorithm>
#include <cstddef>
#include <format>
#include <fstream>
#include <iomanip>
#include <ios>
#include <memory>
#include <mutex>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <utility>
#include <vector>
#include "config.h"
#include "eyeofsauron_db.h"
#include "frame_source.hpp"
#include "util.hpp"

namespace tracker {

Backend::Backend(QObject* parent)
    : QObject(parent),
      _frameWidth(db::Main::videoWidth()),
      _frameHeight(db::Main::videoHeight()),
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
    std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

    if (!pause_preview && !exiting) {
      // QMetaObject::invokeMethod(
      //     this,
      //     [this, frame] {
      //       input_video_frame = frame;
      //       process_frame();
      //     },
      //     Qt::QueuedConnection);
      input_video_frame = frame;
      process_frame();
    }
  });

  connect(media_player_video_sink.get(), &QVideoSink::videoFrameChanged, [this](const QVideoFrame& frame) {
    std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

    if (!pause_preview && !exiting) {
      // QMetaObject::invokeMethod(
      //     this,
      //     [this, frame] {
      //       input_video_frame = frame;
      //       process_frame();
      //     },
      //     Qt::QueuedConnection);
      input_video_frame = frame;
      process_frame();
    }
  });

  connect(media_player.get(), &QMediaPlayer::positionChanged, [this](const qint64& value) {
    _playerPosition = value;

    Q_EMIT playerPositionChanged();
  });

  connect(media_player.get(), &QMediaPlayer::durationChanged, [this](const qint64& value) {
    _playerDuration = value;

    Q_EMIT playerDurationChanged();
  });

  connect(db::Main::self(), &db::Main::videoWidthChanged, [this]() {
    std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

    _frameWidth = db::Main::videoWidth();

    Q_EMIT frameWidthChanged();
  });

  connect(db::Main::self(), &db::Main::videoHeightChanged, [this]() {
    std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

    _frameHeight = db::Main::videoHeight();

    Q_EMIT frameHeightChanged();
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

  std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

  util::debug("Tracker backend exiting...");

  exiting = true;
}

void Backend::start() {
  pause_preview = false;

  switch (current_source_type) {
    case Camera: {
      camera->start();

      break;
    }
    case MediaFile: {
      media_player->play();
      break;
    }
    case Microphone: {
      break;
    }
  }
}

void Backend::pause() {
  pause_preview = true;

  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      media_player->pause();
      break;
    }
    case Microphone: {
      break;
    }
  }
}

void Backend::stop() {
  initial_time = 0;

  switch (current_source_type) {
    case Camera: {
      camera->stop();
      break;
    }
    case MediaFile: {
      media_player->stop();
      break;
    }
    case Microphone: {
      break;
    }
  }
}

void Backend::append(const QUrl& videoUrl) {
  if (videoUrl.isLocalFile()) {
    sourceModel.append(std::make_shared<MediaFileSource>(videoUrl));
  }
}

void Backend::selectSource(const int& index) {
  auto source = sourceModel.get_source(index);

  media_player->stop();
  camera->stop();
  trackers.clear();

  pause_preview = false;

  switch (source->source_type) {
    case Camera: {
      current_source_type = SourceType::Camera;

      _showPlayerSlider = false;

      auto device = dynamic_cast<const CameraSource*>(source.get())->device;

      camera->setCameraDevice(dynamic_cast<const CameraSource*>(source.get())->device);
      camera->setCameraFormat(dynamic_cast<const CameraSource*>(source.get())->format);

      break;
    }
    case MediaFile: {
      current_source_type = SourceType::MediaFile;

      _showPlayerSlider = true;

      auto url = dynamic_cast<const MediaFileSource*>(source.get())->url;

      media_player->setSource(url);

      break;
    }
    case Microphone: {
      break;
    }
  }

  Q_EMIT showPlayerSliderChanged();
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

      auto resolution = std::format("{0}x{1}:{2}", formats.front().resolution().width(),
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
  std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

  cv::Rect2d roi = {x, y, width, height};  // region of interest that is being created

  cv::Ptr<cv::legacy::Tracker> tracker;

  switch (db::Main::trackingAlgorithm()) {
    case db::Main::EnumTrackingAlgorithm::mosse: {
      tracker = cv::legacy::TrackerMOSSE::create();
      break;
    }
    case db::Main::EnumTrackingAlgorithm::kcf: {
      tracker = cv::legacy::TrackerKCF::create();
      break;
    }
    case db::Main::EnumTrackingAlgorithm::tld: {
      tracker = cv::legacy::TrackerTLD::create();
      break;
    }
    case db::Main::EnumTrackingAlgorithm::mil: {
      tracker = cv::legacy::TrackerMIL::create();
      break;
    }
    default: {
      util::warning("Unknown tracking algorithm choice!");
      return;
    }
  }

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
  std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

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

void Backend::removeAllTrackers() {
  std::lock_guard<std::mutex> trackers_lock_guard(trackers_mutex);

  trackers.clear();
}

void Backend::process_frame() {
  if (_videoSink == nullptr) {
    util::warning("Invalid videoSink pointer!");
  }

  if (!input_video_frame.isValid()) {
    util::warning("QVideoFrame is not valid or not writable");

    return;
  }

  auto input_image =
      input_video_frame.toImage()
          .scaled(_frameWidth, _frameHeight, Qt::IgnoreAspectRatio,
                  db::Main::imageScalingAlgorithm() == 0 ? Qt::FastTransformation : Qt::SmoothTransformation)
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

  if (!trackers.empty()) {
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

      double xc = roi_n.x + (roi_n.width * 0.5);
      double yc = roi_n.y + (roi_n.height * 0.5);
      double t = static_cast<double>(input_video_frame.startTime() - initial_time) / 1000000.0;

      // changing the coordinate system origin to the bottom left corner

      yc = _frameHeight - yc;

      data_tx.append(QPointF(t, xc));
      data_ty.append(QPointF(t, yc));

      while (data_tx.size() > db::Main::chartDataPoints()) {
        data_tx.removeFirst();
        data_ty.removeFirst();
      }
    }
  }

  painter.setPen(QColorConstants::Red);

  if (db::Main::showFps()) {
    painter.drawText(output_image.rect(), Qt::AlignLeft | Qt::AlignBottom,
                     QString::fromStdString(std::format(
                         "{0:.0f} fps", 1000000.0 / (input_video_frame.endTime() - input_video_frame.startTime()))));
  }

  if (db::Main::showDateTime()) {
    painter.drawText(output_image.rect(), Qt::AlignLeft | Qt::AlignTop, QDateTime::currentDateTime().toString());
  }

  if (draw_roi_selection) {
    painter.drawRect(rect_selection);

    painter.drawText(
        rect_selection.x(), rect_selection.y(),
        QString::fromStdString(std::format("{0:.0f} x {1:.0f}", rect_selection.width(), rect_selection.height())));
  }

  painter.end();

  // sending the output qvideoframe to the video sink

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);

  if (!pause_preview) {
    update_chart_range();
    Q_EMIT updateChart();
  }
}

void Backend::updateSeries(QAbstractSeries* series_x, QAbstractSeries* series_y, const int& index) {
  if (series_x != nullptr && series_y != nullptr) {
    auto xySeries_x = dynamic_cast<QXYSeries*>(series_x);
    auto xySeries_y = dynamic_cast<QXYSeries*>(series_y);

    auto& [tracker, roi_n, initialized, data_tx, data_ty] = trackers[index];

    if (data_tx.empty() || data_ty.empty()) {
      return;
    }

    // Use replace instead of clear + append, it's optimized for performance
    xySeries_x->replace(data_tx);
    xySeries_y->replace(data_ty);
  } else {
    util::warning("series_x or series_y is null!");
  }
}

void Backend::update_chart_range() {
  double x_axis_min = 0;
  double x_axis_max = 0;
  double y_axis_min = 0;
  double y_axis_max = 0;

  for (size_t n = 0; n < trackers.size(); n++) {
    const auto& [tracker, roi, initialized, data_tx, data_ty] = trackers[n];

    auto [min_x, max_x] = std::ranges::minmax_element(data_tx, [](QPointF a, QPointF b) { return a.y() < b.y(); });
    auto [min_y, max_y] = std::ranges::minmax_element(data_ty, [](QPointF a, QPointF b) { return a.y() < b.y(); });

    auto get_y_range = [this, min_x, min_y, max_x, max_y]() {
      double r_min = 0;
      double r_max = 0;

      if (_xDataVisible && _yDataVisible) {
        r_min = std::min(min_x->y(), min_y->y());
        r_max = std::max(max_x->y(), max_y->y());
      } else if (!_xDataVisible && _yDataVisible) {
        r_min = min_y->y();
        r_max = max_y->y();
      } else {
        r_min = min_x->y();
        r_max = max_x->y();
      }

      return std::make_pair(r_min, r_max);
    };

    auto r = get_y_range();

    if (n == 0) {
      y_axis_min = r.first;
      y_axis_max = r.second;
    } else {
      y_axis_min = std::min(r.first, y_axis_min);
      y_axis_max = std::max(r.second, y_axis_max);
    }

    // calculating the time axis range

    auto [min_t, max_t] = std::ranges::minmax_element(data_tx, [](QPointF a, QPointF b) { return a.x() < b.x(); });

    if (n == 0) {
      x_axis_min = min_t->x();
      x_axis_max = max_t->x();
    } else {
      x_axis_min = std::min(min_t->x(), x_axis_min);
      x_axis_max = std::max(max_t->x(), x_axis_max);
    }
  }

  _xAxisMin = x_axis_min;
  _xAxisMax = x_axis_max;
  _yAxisMin = y_axis_min;
  _yAxisMax = y_axis_max;

  Q_EMIT xAxisMinChanged();
  Q_EMIT xAxisMaxChanged();
  Q_EMIT yAxisMinChanged();
  Q_EMIT yAxisMaxChanged();
}

void Backend::saveTable(const QUrl& fileUrl) {
  if (trackers.empty()) {
    return;
  }

  if (fileUrl.isLocalFile()) {
    QList<QList<QPointF>> list_tx;
    QList<QList<QPointF>> list_ty;

    for (const auto& [tracker, roi, initialized, data_tx, data_ty] : trackers) {
      list_tx.emplace_back(data_tx);
      list_ty.emplace_back(data_ty);
    }

    if (list_tx[0].empty() || list_ty[0].empty()) {
      return;
    }

    const auto N_ROWS = list_tx[0].size();

    std::vector<std::vector<double>> table;

    for (int n = 0; n < N_ROWS; n++) {
      std::vector<double> row;

      row.emplace_back(list_tx[0][n].x());  // time

      for (int m = 0; m < list_tx.size(); m++) {
        row.emplace_back(list_tx[m][n].y());  // x coord
        row.emplace_back(list_ty[m][n].y());  // y coord
      }

      table.emplace_back(row);
    }

    std::ofstream output_file(fileUrl.toLocalFile().toStdString());

    output_file << std::fixed << std::setprecision(db::Main::tableFilePrecision()) << "#time";

    for (size_t k = 0; k < trackers.size(); k++) {
      output_file << std::format("\tx{0}\ty{0}", k);
    }

    output_file << "\n";

    for (const auto& row : table) {
      for (const auto& v : row) {
        output_file << std::format("{0:.{1}f}", v, db::Main::tableFilePrecision()) << "\t";
      }

      output_file << "\n";
    }

    output_file.close();
  }
}

void Backend::setPlayerPosition(qint64 value) {
  initial_time = 0;

  media_player->setPosition(value);
}

}  // namespace tracker