#pragma once

#include <qcameradevice.h>
#include <qstring.h>
#include <qurl.h>

enum SourceType { Camera, MediaFile };

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

class MediaFileSource : public Source {
 public:
  MediaFileSource(QUrl file_url);

  QUrl url;

  QString file_size_mb;

  QString duration;

  QString frame_rate;
};