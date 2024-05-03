#include "tracker.hpp"
#include <qqml.h>
#include <qsize.h>
#include <QPainter>
#include <QVideoFrame>
#include "config.h"
#include "util.hpp"

namespace tracker {

Backend::Backend(QObject* parent) : QObject(parent) {
  qmlRegisterSingletonInstance<Backend>("EoSTrackerBackend", VERSION_MAJOR, VERSION_MINOR, "EoSTrackerBackend", this);

  connect(this, &Backend::videoSinkChanged, [this]() { create_qvideo_frame(); });
}

Backend::~Backend() {
  _videoSink = nullptr;
}

void Backend::start() {
  util::warning("start");

  create_qvideo_frame();
}

void Backend::pause() {
  util::warning("pause");
}

void Backend::stop() {
  util::warning("stop");
}

void Backend::create_qvideo_frame() {
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

  QImage offline(":/images/offline.png");

  QPainter painter(&image);

  painter.drawImage(image.rect(), offline);
  painter.drawText(image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
  painter.end();

  video_frame.unmap();

  _videoSink->setVideoFrame(video_frame);
}

}  // namespace tracker