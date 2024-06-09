#pragma once

#include <qabstractitemmodel.h>
#include <qaudioformat.h>
#include <qbytearray.h>
#include <qcameradevice.h>
#include <qhash.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qurl.h>
#include <qvariant.h>
#include <QAudioDevice>
#include <memory>

enum SourceType { Camera, MediaFile, Microphone };

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

class MicSource : public Source {
 public:
  MicSource(QAudioDevice dev);

  QAudioDevice device;
};

class SourceModel : public QAbstractListModel {
  Q_OBJECT;

 public:
  enum Roles { SourceType = Qt::UserRole, Name, Subtitle, Icon = Qt::DecorationRole };

  [[nodiscard]] int rowCount(const QModelIndex& /*parent*/) const override;

  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

  auto getList() -> QList<std::shared_ptr<Source>>;

  auto get_source(const int& rowIndex) -> std::shared_ptr<Source>;

  void reset();

  void append(std::shared_ptr<Source> source);

  Q_INVOKABLE void removeSource(const int& rowIndex);

 private:
  QList<std::shared_ptr<Source>> list;
};
