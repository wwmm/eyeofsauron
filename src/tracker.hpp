#pragma once

#include <memory.h>
#include <qabstractitemmodel.h>
#include <qobject.h>
#include <QCamera>
#include <QVideoSink>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>

namespace tracker {

class SourceModel : public QAbstractListModel {
  Q_OBJECT;

 public:
  enum TrackerSourceType { Camera, VideoFile };

  struct TrackerSource {
    TrackerSourceType sourceType;

    QCameraDevice cameraDevice;

    QCameraFormat cameraFormat;

    QString videoFilePath;
  };

  enum Roles { Value = Qt::UserRole, SourceIcon = Qt::DecorationRole };

  [[nodiscard]] int rowCount(const QModelIndex& /*parent*/) const override;

  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

  auto getValue(const int& id) -> TrackerSource;

  auto getList() -> QList<TrackerSource>;

  void reset();

  Q_INVOKABLE void append(const struct TrackerSource& source);

  Q_INVOKABLE void remove(const int& rowIndex);

 private:
  QList<TrackerSource> list;
};

class Backend : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVideoSink* videoSink MEMBER _videoSink NOTIFY videoSinkChanged)

  Q_PROPERTY(int sourceIndex MEMBER _sourceIndex NOTIFY sourceIndexChanged)

  Q_PROPERTY(int frameWidth MEMBER _frameWidth NOTIFY frameWidthChanged)

  Q_PROPERTY(int frameHeight MEMBER _frameHeight NOTIFY frameHeightChanged)

 public:
  Backend(QObject* parent = nullptr);

  ~Backend() override;

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void onNewRoi(double x, double y, double width, double height);
  Q_INVOKABLE void onNewRoiSelection(double x, double y, double width, double height);

 signals:
  void videoSinkChanged();
  void sourceIndexChanged();
  void frameWidthChanged();
  void frameHeightChanged();

 private:
  int _frameWidth = 640;
  int _frameHeight = 480;
  int _sourceIndex = 0;

  qint64 initial_time = 0;

  QVideoSink* _videoSink = nullptr;

  QRectF rect_selection = {0.0, 0.0, 0.0, 0.0};

  SourceModel sourceModel;

  std::unique_ptr<QCamera> camera;
  std::unique_ptr<QVideoSink> camera_video_sink;
  std::unique_ptr<QMediaCaptureSession> capture_session;

  std::vector<std::tuple<cv::Ptr<cv::legacy::TrackerMOSSE>, cv::Rect2d, bool>> trackers;

  void find_best_camera_resolution();
  void draw_offline_image();
  void process_frame(const QVideoFrame& input_frame);
};

}  // namespace tracker