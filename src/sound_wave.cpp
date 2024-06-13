#include "sound_wave.hpp"
#include <qabstractseries.h>
#include <qaudiodevice.h>
#include <qaudioformat.h>
#include <qaudiosource.h>
#include <qlogging.h>
#include <qmediacapturesession.h>
#include <qmediaplayer.h>
#include <qobject.h>
#include <qqml.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qxyseries.h>
#include <QMediaDevices>
#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>
#include "config.h"
#include "eyeofsauron_db.h"
#include "frame_source.hpp"
#include "io_device.hpp"
#include "util.hpp"

namespace sound {

Backend::Backend(QObject* parent)
    : QObject(parent),
      io_device(std::make_unique<IODevice>()),
      capture_session(std::make_unique<QMediaCaptureSession>()),
      media_player(std::make_unique<QMediaPlayer>()) {
  qmlRegisterSingletonInstance<Backend>("EoSSoundBackend", VERSION_MAJOR, VERSION_MINOR, "EoSSoundBackend", this);

  qmlRegisterSingletonInstance<SourceModel>("EosSoundSourceModel", VERSION_MAJOR, VERSION_MINOR, "EosSoundSourceModel",
                                            &sourceModel);

  connect(io_device.get(), &IODevice::bufferChanged, [this](const std::vector<double>& buffer) {
    std::lock_guard<std::mutex> microphone_lock_guard(microphone_mutex);

    process_buffer(buffer);
  });

  io_device->open(QIODevice::WriteOnly);

  find_microphones();
}

Backend::~Backend() {
  microphone->stop();
  media_player->stop();

  exiting = true;
}

void Backend::start() {
  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      media_player->play();
      break;
    }
    case Microphone: {
      microphone->start(io_device.get());
      break;
    }
  }
}

void Backend::pause() {
  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      media_player->pause();
      break;
    }
    case Microphone: {
      microphone->stop();
      break;
    }
  }
}

void Backend::stop() {
  time_axis = 0;

  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      media_player->stop();
      break;
    }
    case Microphone: {
      microphone->stop();
      break;
    }
  }
}

void Backend::append(const QUrl& mediaUrl) {
  if (mediaUrl.isLocalFile()) {
    sourceModel.append(std::make_shared<MediaFileSource>(mediaUrl));
  }
}

void Backend::selectSource(const int& index) {
  auto source = sourceModel.get_source(index);

  if (microphone != nullptr) {
    microphone->stop();
  }

  media_player->stop();

  switch (source->source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      current_source_type = SourceType::MediaFile;

      _showPlayerSlider = true;

      auto url = dynamic_cast<const MediaFileSource*>(source.get())->url;

      media_player->setSource(url);
      media_player->play();

      break;
    }
    case Microphone: {
      auto device = dynamic_cast<const MicSource*>(source.get())->device;

      // https:  // code.qt.io/cgit/qt/qtcharts.git/tree/examples/charts/audio/widget.cpp?h=6.7

      auto format = device.preferredFormat();
      format.setSampleFormat(QAudioFormat::Float);
      format.setChannelCount(1);

      io_device->set_format(format);

      std::lock_guard<std::mutex> microphone_lock_guard(microphone_mutex);

      microphone = std::make_unique<QAudioSource>(device, format);

      microphone->start(io_device.get());

      break;
    }
  }

  Q_EMIT showPlayerSliderChanged();
}

void Backend::find_microphones() {
  for (const auto& device : QMediaDevices::audioInputs()) {
    if (!device.isNull()) {
      sourceModel.append(std::make_shared<MicSource>(device));
    }
  }
}

void Backend::process_buffer(const std::vector<double>& buffer) {
  double dt = 1.0 / microphone->format().sampleRate();

  for (double v : buffer) {
    waveform.append(QPointF(time_axis, v));

    time_axis += dt;

    // qDebug() << v;
  }

  while ((waveform.size() - 1) * dt > db::Main::chartTimeWindow()) {
    waveform.removeFirst();
    waveform.removeFirst();
  }

  update_waveformt_chart_range();

  Q_EMIT updateChart();
}

void Backend::update_waveformt_chart_range() {
  auto [min_x, max_x] = std::ranges::minmax_element(waveform, [](QPointF a, QPointF b) { return a.x() < b.x(); });
  auto [min_y, max_y] = std::ranges::minmax_element(waveform, [](QPointF a, QPointF b) { return a.y() < b.y(); });

  _xAxisMinWave = min_x->x();
  _xAxisMaxWave = max_x->x();
  _yAxisMinWave = min_y->y();
  _yAxisMaxWave = max_y->y();

  Q_EMIT xAxisMinWaveChanged();
  Q_EMIT xAxisMaxWaveChanged();
  Q_EMIT yAxisMinWaveChanged();
  Q_EMIT yAxisMaxWaveChanged();
}

void Backend::updateSeriesWaveform(QAbstractSeries* series) {
  if (series != nullptr) {
    auto xySeries = dynamic_cast<QXYSeries*>(series);

    if (waveform.empty()) {
      return;
    }

    // Use replace instead of clear + append, it's optimized for performance
    xySeries->replace(waveform);
  } else {
    util::warning("series waveform is null!");
  }
}

void Backend::saveTable(const QUrl& fileUrl) {}

void Backend::setPlayerPosition(qint64 value) {
  time_axis = 0;

  media_player->setPosition(value);
}

}  // namespace sound