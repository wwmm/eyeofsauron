#pragma once

#include <qcameradevice.h>
#include <qurl.h>

enum SourceType { Camera, VideoFile };

class Source {
 public:
  virtual ~Source() = default;

  SourceType source_type;

 protected:
  Source(SourceType source_type);
};

class CameraSource : public Source {
 public:
  CameraSource(QCameraDevice dev, QCameraFormat fmt);

  QCameraDevice device;

  QCameraFormat format;
};

class VideoFileSource : public Source {
 public:
  VideoFileSource(QUrl file_url);

  QUrl url;
};