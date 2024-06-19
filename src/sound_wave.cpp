#include "sound_wave.hpp"
#include <fftw3.h>
#include <qabstractseries.h>
#include <qaudiodecoder.h>
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
#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <fstream>
#include <iomanip>
#include <ios>
#include <memory>
#include <mutex>
#include <numbers>
#include <ratio>
#include <regex>
#include <span>
#include <thread>
#include <vector>
#include "config.h"
#include "eyeofsauron_db.h"
#include "frame_source.hpp"
#include "io_device.hpp"
#include "util.hpp"

namespace sound {

Backend::Backend(QObject* parent)
    : QObject(parent), io_device(std::make_unique<IODevice>()), decoder(std::make_unique<QAudioDecoder>()) {
  qmlRegisterSingletonInstance<Backend>("EoSSoundBackend", VERSION_MAJOR, VERSION_MINOR, "EoSSoundBackend", this);

  qmlRegisterSingletonInstance<SourceModel>("EosSoundSourceModel", VERSION_MAJOR, VERSION_MINOR, "EosSoundSourceModel",
                                            &sourceModel);

  connect(io_device.get(), &IODevice::bufferChanged, [this](const std::vector<double>& buffer) {
    std::lock_guard<std::mutex> microphone_lock_guard(microphone_mutex);

    process_buffer(buffer, microphone->format().sampleRate());
  });

  connect(decoder.get(), &QAudioDecoder::positionChanged, [this](const qint64& value) {
    _playerPosition = value;

    // util::warning(util::to_string(value));

    Q_EMIT playerPositionChanged();
  });

  connect(decoder.get(), &QAudioDecoder::durationChanged, [this](const qint64& value) {
    _playerDuration = value;

    Q_EMIT playerDurationChanged();
  });

  connect(decoder.get(), &QAudioDecoder::bufferReady, [this]() {
    auto qaudio_buffer = decoder->read();

    if (decoder_buffer.size() != static_cast<size_t>(qaudio_buffer.sampleCount())) {
      decoder_buffer.resize(qaudio_buffer.sampleCount());
    }

    auto input_data = std::span<const float>(qaudio_buffer.constData<float>(), qaudio_buffer.sampleCount());

    std::copy(input_data.begin(), input_data.end(), decoder_buffer.begin());

    if (qaudio_buffer.startTime() == 0) {
      first_buffer_clock = std::chrono::steady_clock::now();
    }

    std::chrono::duration<double, std::micro> diff = std::chrono::steady_clock::now() - first_buffer_clock;

    auto elapsed_time = diff.count();

    while (static_cast<qint64>(elapsed_time) < qaudio_buffer.startTime()) {
      std::this_thread::sleep_for(std::chrono::microseconds(1));

      std::chrono::duration<double, std::micro> diff = std::chrono::steady_clock::now() - first_buffer_clock;

      elapsed_time = diff.count();
    }

    // qDebug() << static_cast<qint64>(elapsed_time) << qaudio_buffer.startTime();

    process_buffer(decoder_buffer, qaudio_buffer.format().sampleRate());
  });

  io_device->open(QIODevice::WriteOnly);

  QAudioFormat format;

  format.setSampleFormat(QAudioFormat::Float);
  format.setChannelCount(1);

  decoder->setAudioFormat(format);

  find_microphones();
}

Backend::~Backend() {
  if (microphone != nullptr) {
    microphone->stop();
  }

  decoder->stop();

  exiting = true;
}

void Backend::start() {
  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      decoder->start();
      break;
    }
    case Microphone: {
      if (microphone != nullptr) {
        microphone->start(io_device.get());
      }
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
      decoder->stop();
      break;
    }
    case Microphone: {
      if (microphone != nullptr) {
        microphone->stop();
      }
      break;
    }
  }
}

void Backend::stop() {
  time_axis = 0;

  waveform.clear();
  fft_list.clear();

  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      decoder->stop();
      break;
    }
    case Microphone: {
      if (microphone != nullptr) {
        microphone->stop();
      }
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

  decoder->stop();

  switch (source->source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      current_source_type = SourceType::MediaFile;

      _showPlayerSlider = true;

      auto url = dynamic_cast<const MediaFileSource*>(source.get())->url;

      decoder->setSource(url);

      decoder->start();

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

void Backend::calc_fft(const int& sampling_rate) {
  if (waveform.empty()) {
    return;
  }

  fft_list.resize(waveform.size() / 2U + 1U);

  real_input.resize(0);

  for (const auto& p : waveform) {
    real_input.emplace_back(p.y());
  }

  for (uint n = 0U; n < real_input.size(); n++) {
    // https://en.wikipedia.org/wiki/Hann_function

    const float w = 0.5F * (1.0F - std::cos(2.0F * std::numbers::pi_v<float> * static_cast<float>(n) /
                                            static_cast<float>(real_input.size() - 1U)));

    real_input[n] *= w;
  }

  auto* complex_output = fftw_alloc_complex(real_input.size());

  auto* plan =
      fftw_plan_dft_r2c_1d(static_cast<int>(real_input.size()), real_input.data(), complex_output, FFTW_ESTIMATE);

  fftw_execute(plan);

  for (uint i = 0U; i < fft_list.size(); i++) {
    double sqr = complex_output[i][0] * complex_output[i][0] + complex_output[i][1] * complex_output[i][1];

    sqr /= static_cast<double>(fft_list.size() * fft_list.size());

    double f = 0.5F * static_cast<float>(sampling_rate) * static_cast<float>(i) / static_cast<float>(fft_list.size());

    fft_list[i] = QPointF(f, sqr);
  }

  // removing the DC component at f = 0 Hz

  fft_list.erase(fft_list.begin());

  if (complex_output != nullptr) {
    fftw_free(complex_output);
  }

  fftw_destroy_plan(plan);
}

void Backend::process_buffer(const std::vector<double>& buffer, const int& sampling_rate) {
  double dt = 1.0 / sampling_rate;

  for (double v : buffer) {
    waveform.append(QPointF(time_axis, v));

    time_axis += dt;
  }

  while ((waveform.size() - 1) * dt > db::Main::chartTimeWindow()) {
    waveform.removeFirst();
    waveform.removeFirst();
  }

  calc_fft(sampling_rate);

  update_waveform_chart_range();
  update_fft_chart_range();

  Q_EMIT updateChart();
}

void Backend::update_waveform_chart_range() {
  if (waveform.empty()) {
    return;
  }

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

void Backend::update_fft_chart_range() {
  if (fft_list.empty()) {
    return;
  }

  auto [min_x, max_x] = std::ranges::minmax_element(fft_list, [](QPointF a, QPointF b) { return a.x() < b.x(); });
  auto [min_y, max_y] = std::ranges::minmax_element(fft_list, [](QPointF a, QPointF b) { return a.y() < b.y(); });

  _xAxisMinFFT = min_x->x();
  _xAxisMaxFFT = max_x->x();
  _yAxisMinFFT = min_y->y();
  _yAxisMaxFFT = max_y->y();

  Q_EMIT xAxisMinFFTChanged();
  Q_EMIT xAxisMaxFFTChanged();
  Q_EMIT yAxisMinFFTChanged();
  Q_EMIT yAxisMaxFFTChanged();
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

void Backend::updateSeriesFFT(QAbstractSeries* series) {
  if (series != nullptr) {
    auto xySeries = dynamic_cast<QXYSeries*>(series);

    if (fft_list.empty()) {
      return;
    }

    // Use replace instead of clear + append, it's optimized for performance
    xySeries->replace(fft_list);
  } else {
    util::warning("series fft is null!");
  }
}

void Backend::saveTable(const QUrl& fileUrl) {
  if (waveform.empty() || fft_list.empty()) {
    return;
  }

  if (fileUrl.isLocalFile()) {
    {  // waveform
      std::ofstream output_file(
          std::regex_replace(fileUrl.toLocalFile().toStdString(), std::regex(".tsv"), "_waveform.tsv"));

      output_file << "#time\tvalue\n";

      for (const auto& p : waveform) {
        output_file << std::format("{1:.{0}e}\t{2:.{0}e}", _tableFilePrecision, p.x(), p.y()) << "\n";
      }

      output_file.close();
    }

    {  // fft
      std::ofstream output_file(
          std::regex_replace(fileUrl.toLocalFile().toStdString(), std::regex(".tsv"), "_fft.tsv"));

      output_file << "#frequency\tvalue\n";

      for (const auto& p : fft_list) {
        output_file << std::format("{1:.{0}e}\t{2:.{0}e}", _tableFilePrecision, p.x(), p.y()) << "\n";
      }

      output_file.close();
    }
  }
}

void Backend::setPlayerPosition(qint64 value) {
  time_axis = 0;

  // decoder->setPosition(value);
}

}  // namespace sound