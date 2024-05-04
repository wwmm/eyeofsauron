#include "tracker.hpp"
#include <qqml.h>
#include <qsize.h>
#include <QMediaCaptureSession>
#include <QPainter>
#include <QVideoFrame>
#include "config.h"
#include "util.hpp"

namespace tracker {

Backend::Backend(QObject* parent)
    : QObject(parent),
      camera(std::make_unique<QCamera>()),
      camera_video_sink(std::make_unique<QVideoSink>()),
      capture_session(std::make_unique<QMediaCaptureSession>()) {
  qmlRegisterSingletonInstance<Backend>("EoSTrackerBackend", VERSION_MAJOR, VERSION_MINOR, "EoSTrackerBackend", this);

  capture_session->setCamera(camera.get());
  capture_session->setVideoSink(camera_video_sink.get());

  connect(this, &Backend::videoSinkChanged, [this]() { draw_offline_image(); });

  connect(camera_video_sink.get(), &QVideoSink::videoFrameChanged,
          [this](const QVideoFrame& frame) { process_frame(frame); });
}

Backend::~Backend() {
  _videoSink = nullptr;
}

void Backend::start() {
  camera->start();
}

void Backend::stop() {
  camera->stop();
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
  painter.drawText(image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
  painter.end();

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

void Backend::process_frame(const QVideoFrame& input_frame) {
  auto input_image = input_frame.toImage();

  auto video_format =
      QVideoFrameFormat(QSize(input_image.width(), input_image.height()), QVideoFrameFormat::Format_RGBA8888);

  video_format.setColorRange(QVideoFrameFormat::ColorRange_Full);

  QVideoFrame video_frame(video_format);

  if (!video_frame.isValid() || !video_frame.map(QVideoFrame::WriteOnly)) {
    util::warning("QVideoFrame is not valid or not writable");

    return;
  }

  QImage output_image(video_frame.bits(0), video_frame.width(), video_frame.height(), QImage::Format_RGBA8888);

  QPainter painter(&output_image);

  painter.drawImage(output_image.rect(), input_image);
  painter.drawText(output_image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
  painter.end();

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

void Backend::create_qvideo_frame(const QImage& main_frame) {
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

  if (image.isNull()) {
    util::warning("The created QImage object is null!");

    return;
  }

  // image.fill(QColorConstants::White);

  QPainter painter(&image);

  painter.drawImage(image.rect(), main_frame);
  painter.drawText(image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
  painter.end();

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

}  // namespace tracker