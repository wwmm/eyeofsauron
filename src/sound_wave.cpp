#include "sound_wave.hpp"
#include <qaudiodevice.h>
#include <qaudiosink.h>
#include <qmediacapturesession.h>
#include <qmediaplayer.h>
#include <qobject.h>
#include <qqml.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <QMediaDevices>
#include <memory>
#include "config.h"
#include "frame_source.hpp"

namespace sound {

Backend::Backend(QObject* parent)
    : QObject(parent),
      microphone(std::make_unique<QAudioDevice>()),
      microphone_audio_sink(std::make_unique<QAudioSink>()),
      capture_session(std::make_unique<QMediaCaptureSession>()),
      media_player(std::make_unique<QMediaPlayer>()),
      media_player_audio_sink(std::make_unique<QAudioSink>()) {
  qmlRegisterSingletonInstance<Backend>("EoSSoundBackend", VERSION_MAJOR, VERSION_MINOR, "EoSSoundBackend", this);

  qmlRegisterSingletonInstance<SourceModel>("EosSoundSourceModel", VERSION_MAJOR, VERSION_MINOR, "EosSoundSourceModel",
                                            &sourceModel);

  find_microphones();
}

Backend::~Backend() {
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
      break;
    }
  }
}

void Backend::stop() {
  initial_time = 0;

  switch (current_source_type) {
    case Camera: {
      break;
    }
    case MediaFile: {
      media_player->stop();
      break;
    }
    case Microphone: {
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
      break;
    }
  }

  Q_EMIT showPlayerSliderChanged();
}

void Backend::find_microphones() {
  for (const auto& device : QMediaDevices::audioInputs()) {
    if (!device.isNull()) {
      qDebug() << device.description() << device.preferredFormat() << device.supportedSampleFormats();

      sourceModel.append(std::make_shared<MicSource>(device));
    }
  }
}

void Backend::saveTable(const QUrl& fileUrl) {}

void Backend::setPlayerPosition(qint64 value) {
  initial_time = 0;

  media_player->setPosition(value);
}

}  // namespace sound