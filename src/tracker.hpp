#pragma once

#include <qabstractseries.h>
#include <qassert.h>
#include <qjsengine.h>
#include <qlist.h>
#include <qobject.h>
#include <qpoint.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtpreprocessorsupport.h>
#include <qtypes.h>
#include <qurl.h>
#include <QCamera>
#include <QMediaPlayer>
#include <QSplineSeries>
#include <QVideoSink>
#include <memory>
#include <mutex>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>  // IWYU pragma: export
#include <tuple>
#include <vector>
#include "frame_source.hpp"

namespace tracker {

class Backend : public QObject {
  Q_OBJECT

  QML_NAMED_ELEMENT(EoSTrackerBackend)
  QML_SINGLETON

  Q_PROPERTY(bool xDataVisible MEMBER _xDataVisible NOTIFY xDataVisibleChanged)

  Q_PROPERTY(bool yDataVisible MEMBER _yDataVisible NOTIFY yDataVisibleChanged)

  Q_PROPERTY(int showPlayerSlider MEMBER _showPlayerSlider NOTIFY showPlayerSliderChanged)

  Q_PROPERTY(int frameWidth MEMBER _frameWidth NOTIFY frameWidthChanged)

  Q_PROPERTY(int frameHeight MEMBER _frameHeight NOTIFY frameHeightChanged)

  Q_PROPERTY(int playerPosition MEMBER _playerPosition NOTIFY playerPositionChanged)

  Q_PROPERTY(int playerDuration MEMBER _playerDuration NOTIFY playerDurationChanged)

  Q_PROPERTY(double xAxisMin MEMBER _xAxisMin NOTIFY xAxisMinChanged)

  Q_PROPERTY(double xAxisMax MEMBER _xAxisMax NOTIFY xAxisMaxChanged)

  Q_PROPERTY(double yAxisMin MEMBER _yAxisMin NOTIFY yAxisMinChanged)

  Q_PROPERTY(double yAxisMax MEMBER _yAxisMax NOTIFY yAxisMaxChanged)

  Q_PROPERTY(QVideoSink* videoSink MEMBER _videoSink NOTIFY videoSinkChanged)

  Q_PROPERTY(SourceModel* sourceModel MEMBER sourceModel CONSTANT)

 public:
  explicit Backend(QObject* parent = nullptr);

  /**
   * Deleting the default constructor because we want Qt to call our custom create method.
   * If this is not done qml will create its own class instance.
   */
  Backend() = delete;

  Backend(const Backend&) = delete;
  auto operator=(const Backend&) -> Backend& = delete;
  Backend(const Backend&&) = delete;
  auto operator=(const Backend&&) -> Backend& = delete;
  ~Backend() override;

  static Backend& self() {
    static Backend m(nullptr);
    return m;
  }

  inline static Backend* singletonInstance = nullptr;

  // Singleton provider for QML
  static Backend* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(jsEngine)

    // The engine has to have the same thread affinity as the singleton.

    Q_ASSERT(qmlEngine->thread() == self().thread());

    // Explicitly specify C++ ownership so that the engine doesn't delete the instance.

    QJSEngine::setObjectOwnership(&self(), QJSEngine::CppOwnership);

    return &self();
  }

  Q_INVOKABLE void start();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void append(const QUrl& videoUrl);
  Q_INVOKABLE void selectSource(const int& index);
  Q_INVOKABLE void drawRoiSelection(const bool& state);
  Q_INVOKABLE void createNewRoi(double x, double y, double width, double height);
  Q_INVOKABLE void newRoiSelection(double x, double y, double width, double height);
  Q_INVOKABLE int removeRoi(double x, double y);
  Q_INVOKABLE void removeAllTrackers();
  Q_INVOKABLE void updateSeries(QAbstractSeries* series_x, QAbstractSeries* series_y, const int& index);
  Q_INVOKABLE void updateGSeries(QSplineSeries* series_x, QSplineSeries* series_y, const int& index);
  Q_INVOKABLE void saveTable(const QUrl& fileUrl);
  Q_INVOKABLE void setPlayerPosition(qint64 value);

 signals:
  void videoSinkChanged();
  void frameWidthChanged();
  void frameHeightChanged();
  void xAxisMinChanged();
  void xAxisMaxChanged();
  void yAxisMinChanged();
  void yAxisMaxChanged();
  void xDataVisibleChanged();
  void yDataVisibleChanged();
  void playerPositionChanged();
  void playerDurationChanged();
  void showPlayerSliderChanged();
  void updateChart();
  void updateTrackerGraph(int index, QList<QPointF> newDataX, QList<QPointF> newDataY);

 private:
  bool _xDataVisible = true;
  bool _yDataVisible = true;
  bool _showPlayerSlider = false;
  bool draw_roi_selection = false;
  bool pause_preview = false;
  bool exiting = false;

  int _frameWidth = 800;
  int _frameHeight = 600;

  double _xAxisMin = 10000;
  double _xAxisMax = 0;
  double _yAxisMin = 10000;
  double _yAxisMax = 0;

  qint64 initial_time = 0;
  qint64 _playerPosition = 0;
  qint64 _playerDuration = 0;

  SourceType current_source_type = SourceType::Camera;

  QVideoSink* _videoSink = nullptr;

  QVideoFrame input_video_frame;

  QRectF rect_selection = {0.0, 0.0, 0.0, 0.0};

  SourceModel* sourceModel = nullptr;

  std::unique_ptr<QCamera> camera;
  std::unique_ptr<QVideoSink> camera_video_sink;
  std::unique_ptr<QMediaCaptureSession> capture_session;
  std::unique_ptr<QMediaPlayer> media_player;
  std::unique_ptr<QVideoSink> media_player_video_sink;

  std::vector<std::tuple<cv::Ptr<cv::legacy::Tracker>, cv::Rect2d, bool, QList<QPointF>, QList<QPointF>>> trackers;

  std::mutex trackers_mutex;

  void find_best_camera_resolution();
  void draw_offline_image();
  void process_frame();
  void update_chart_range();
};

}  // namespace tracker