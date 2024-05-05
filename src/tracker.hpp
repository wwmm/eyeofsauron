#pragma once

#include <memory.h>
#include <qobject.h>
#include <QCamera>
#include <QVideoSink>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>

namespace tracker {

class Backend : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVideoSink* videoSink MEMBER _videoSink NOTIFY videoSinkChanged)

  Q_PROPERTY(QList<QCameraDevice> listCameraDevice MEMBER _listCameraDevice NOTIFY listCameraDeviceChanged)

  Q_PROPERTY(QList<QCameraFormat> listCameraFormat MEMBER _listCameraFormat NOTIFY listCameraFormatChanged)

  Q_PROPERTY(int cameraIndex MEMBER _cameraIndex NOTIFY cameraIndexChanged)

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
  void listCameraDeviceChanged();
  void listCameraFormatChanged();
  void cameraIndexChanged();
  void frameWidthChanged();
  void frameHeightChanged();

 private:
  int _frameWidth = 640;
  int _frameHeight = 480;
  int _cameraIndex = 0;

  qint64 initial_time = 0;

  QVideoSink* _videoSink = nullptr;

  QRectF rect_selection = {0.0, 0.0, 0.0, 0.0};

  QList<QCameraDevice> _listCameraDevice;
  QList<QCameraFormat> _listCameraFormat;

  std::unique_ptr<QCamera> camera;
  std::unique_ptr<QVideoSink> camera_video_sink;
  std::unique_ptr<QMediaCaptureSession> capture_session;

  std::vector<std::tuple<cv::Ptr<cv::legacy::TrackerMOSSE>, cv::Rect2d, bool>> trackers;

  void find_best_camera_resolution();
  void draw_offline_image();
  void process_frame(const QVideoFrame& input_frame);
};

}  // namespace tracker