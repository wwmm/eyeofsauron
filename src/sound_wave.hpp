#pragma once

#include <qabstractitemmodel.h>
#include <qabstractseries.h>
#include <qbytearray.h>
#include <qhash.h>
#include <qlist.h>
#include <qmediaplayer.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <QAudioDecoder>
#include <QAudioOutput>
#include <QAudioSource>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#include "frame_source.hpp"
#include "io_device.hpp"

namespace sound {

class Backend : public QObject {
  Q_OBJECT

  Q_PROPERTY(int showPlayerSlider MEMBER _showPlayerSlider NOTIFY showPlayerSliderChanged)

  Q_PROPERTY(int playerPosition MEMBER _playerPosition NOTIFY playerPositionChanged)

  Q_PROPERTY(int playerDuration MEMBER _playerDuration NOTIFY playerDurationChanged)

  Q_PROPERTY(double xAxisMinWave MEMBER _xAxisMinWave NOTIFY xAxisMinWaveChanged)

  Q_PROPERTY(double xAxisMaxWave MEMBER _xAxisMaxWave NOTIFY xAxisMaxWaveChanged)

  Q_PROPERTY(double yAxisMinWave MEMBER _yAxisMinWave NOTIFY yAxisMinWaveChanged)

  Q_PROPERTY(double yAxisMaxWave MEMBER _yAxisMaxWave NOTIFY yAxisMaxWaveChanged)

  Q_PROPERTY(double xAxisMinFFT MEMBER _xAxisMinFFT NOTIFY xAxisMinFFTChanged)

  Q_PROPERTY(double xAxisMaxFFT MEMBER _xAxisMaxFFT NOTIFY xAxisMaxFFTChanged)

  Q_PROPERTY(double yAxisMinFFT MEMBER _yAxisMinFFT NOTIFY yAxisMinFFTChanged)

  Q_PROPERTY(double yAxisMaxFFT MEMBER _yAxisMaxFFT NOTIFY yAxisMaxFFTChanged)

  Q_PROPERTY(int tableFilePrecision MEMBER _tableFilePrecision NOTIFY tableFilePrecisionChanged)

 public:
  Backend(QObject* parent = nullptr);

  ~Backend() override;

  Q_INVOKABLE void start();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void append(const QUrl& mediaUrl);
  Q_INVOKABLE void selectSource(const int& index);
  Q_INVOKABLE void updateSeriesWaveform(QAbstractSeries* series);
  Q_INVOKABLE void updateSeriesFFT(QAbstractSeries* series);
  Q_INVOKABLE void saveTable(const QUrl& fileUrl);
  Q_INVOKABLE void setPlayerPosition(qint64 value);

 signals:
  void tableFilePrecisionChanged();
  void xAxisMinWaveChanged();
  void xAxisMaxWaveChanged();
  void yAxisMinWaveChanged();
  void yAxisMaxWaveChanged();
  void xAxisMinFFTChanged();
  void xAxisMaxFFTChanged();
  void yAxisMinFFTChanged();
  void yAxisMaxFFTChanged();
  void playerPositionChanged();
  void playerDurationChanged();
  void showPlayerSliderChanged();
  void updateChart();

 private:
  bool _showPlayerSlider = false;
  bool exiting = false;

  int _tableFilePrecision = 4;

  double _xAxisMinWave = 10000;
  double _xAxisMaxWave = 0;
  double _yAxisMinWave = 10000;
  double _yAxisMaxWave = 0;
  double _xAxisMinFFT = 10000;
  double _xAxisMaxFFT = 0;
  double _yAxisMinFFT = 10000;
  double _yAxisMaxFFT = 0;
  double time_axis = 0;

  qint64 _playerPosition = 0;
  qint64 _playerDuration = 0;

  SourceModel sourceModel;

  SourceType current_source_type = SourceType::Microphone;

  std::unique_ptr<IODevice> io_device;
  std::unique_ptr<QAudioSource> microphone;
  std::unique_ptr<QAudioDecoder> decoder;

  std::mutex microphone_mutex;

  QList<QPointF> waveform;
  QList<QPointF> fft_list;

  std::vector<double> real_input;
  std::vector<double> decoder_buffer;

  std::chrono::time_point<std::chrono::steady_clock> first_buffer_clock;

  void find_microphones();
  void process_buffer(const std::vector<double>& buffer, const int& sampling_rate);
  void calc_fft(const int& sampling_rate);
  void update_waveform_chart_range();
  void update_fft_chart_range();
};

}  // namespace sound