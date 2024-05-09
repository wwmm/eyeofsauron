#pragma once

#include <memory.h>
#include <qabstractitemmodel.h>
#include <qhash.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <QCamera>
#include <QMediaPlayer>
#include <QVideoSink>
#include <memory>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>  // IWYU pragma: export
#include <tuple>
#include <vector>
#include "frame_source.hpp"

namespace tracker {

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

class Backend : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVideoSink* videoSink MEMBER _videoSink NOTIFY videoSinkChanged)

  Q_PROPERTY(int frameWidth MEMBER _frameWidth NOTIFY frameWidthChanged)

  Q_PROPERTY(int frameHeight MEMBER _frameHeight NOTIFY frameHeightChanged)

 public:
  Backend(QObject* parent = nullptr);

  ~Backend() override;

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void onNewRoi(double x, double y, double width, double height);
  Q_INVOKABLE void onNewRoiSelection(double x, double y, double width, double height);
  Q_INVOKABLE void append(const QUrl& videoUrl);
  Q_INVOKABLE void selectSource(const int& index);

 signals:
  void videoSinkChanged();
  void sourceIndexChanged();
  void frameWidthChanged();
  void frameHeightChanged();

 private:
  int _frameWidth = 640;
  int _frameHeight = 480;

  qint64 initial_time = 0;

  SourceType current_source_type = SourceType::Camera;

  QVideoSink* _videoSink = nullptr;

  QRectF rect_selection = {0.0, 0.0, 0.0, 0.0};

  SourceModel sourceModel;

  std::unique_ptr<QCamera> camera;
  std::unique_ptr<QVideoSink> camera_video_sink;
  std::unique_ptr<QMediaCaptureSession> capture_session;
  std::unique_ptr<QMediaPlayer> media_player;
  std::unique_ptr<QVideoSink> media_player_video_sink;

  std::vector<std::tuple<cv::Ptr<cv::legacy::TrackerMOSSE>, cv::Rect2d, bool>> trackers;
  // std::vector<std::tuple<cv::Ptr<cv::TrackerVit>, cv::Rect, bool>> trackers;

  void find_best_camera_resolution();
  void draw_offline_image();
  void process_frame(const QVideoFrame& input_frame);
};

}  // namespace tracker