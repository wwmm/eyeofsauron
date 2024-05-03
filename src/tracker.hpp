#pragma once

#include <qobject.h>
#include <QVideoSink>

namespace tracker {

class Backend : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVideoSink* videoSink MEMBER _videoSink NOTIFY videoSinkChanged)

  Q_PROPERTY(int frameWidth MEMBER _frameWidth NOTIFY frameWidthChanged)

  Q_PROPERTY(int frameHeight MEMBER _frameHeight NOTIFY frameHeightChanged)

 public:
  Backend(QObject* parent = nullptr);

  ~Backend() override;

  // [[nodiscard]] QVideoSink* videoSink() const;

  Q_INVOKABLE void start();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void stop();

 signals:
  void videoSinkChanged();
  void frameWidthChanged();
  void frameHeightChanged();

 private:
  int _frameWidth = 640;
  int _frameHeight = 480;

  QVideoSink* _videoSink = nullptr;

  void create_qvideo_frame();
};

}  // namespace tracker