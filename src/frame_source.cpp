#include "frame_source.hpp"
#include <qabstractitemmodel.h>
#include <qaudiodevice.h>
#include <qaudioformat.h>
#include <qbytearray.h>
#include <qdebug.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qtmetamacros.h>
#include <qvariant.h>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#define _UNICODE
#include <MediaInfo/MediaInfo.h>
#include <MediaInfo/MediaInfo_Const.h>
#include <qcameradevice.h>
#include <qurl.h>
#include <KLocalizedString>
#include "util.hpp"

Source::Source(SourceType source_type) : source_type(source_type) {}

CameraSource::CameraSource(QCameraDevice dev, QCameraFormat fmt)
    : Source(SourceType::Camera), device(dev), format(fmt) {}

MediaFileSource::MediaFileSource(QUrl file_url) : Source(SourceType::MediaFile), url(std::move(file_url)) {
  if (url.isLocalFile()) {
    using namespace MediaInfoLib;

    MediaInfo MI;

    if (MI.Open(url.toLocalFile().toStdWString()) == 1) {
      auto f_size_str =
          QString::fromStdWString(MI.Get(Stream_General, 0, static_cast<String>(L"FileSize"), Info_Text, Info_Name));

      float f_size_bytes = 0;
      float duration_ms = 0;
      float fps = 0;
      float audio_sampling_rate = 0;

      // file size

      util::str_to_num(f_size_str.toStdString(), f_size_bytes);

      file_size_mb = QString::fromStdString(std::format("{0:.1f}", f_size_bytes / 1024 / 1024));

      // duration

      auto duration_str =
          QString::fromStdWString(MI.Get(Stream_General, 0, static_cast<String>(L"Duration"), Info_Text, Info_Name));

      util::str_to_num(duration_str.toStdString(), duration_ms);

      int minutes = std::floor(duration_ms / 60000);

      float seconds = duration_ms / 1000 - minutes * 60;

      duration = QString::fromStdString(std::format("{0:d}:{1:0>4.1f}", minutes, seconds));

      // video fps

      auto frame_rate_str =
          QString::fromStdWString(MI.Get(Stream_General, 0, static_cast<String>(L"FrameRate"), Info_Text, Info_Name));

      util::str_to_num(frame_rate_str.toStdString(), fps);

      frame_rate = QString::fromStdString(std::format("{0:.1f}", fps));

      // audio sampling rate

      auto audio_rate_str =
          QString::fromStdWString(MI.Get(Stream_Audio, 0, static_cast<String>(L"SamplingRate"), Info_Text, Info_Name));

      util::str_to_num(audio_rate_str.toStdString(), audio_sampling_rate);

      audio_rate = QString::fromStdString(std::format("{0:.1f} kHz", audio_sampling_rate / 1000));

      MI.Close();
    } else {
      util::warning("failed to get media information");
    }
  }
}

MicSource::MicSource(QAudioDevice dev) : Source(SourceType::Microphone), device(std::move(dev)) {}

int SourceModel::rowCount(const QModelIndex& /*parent*/) const {
  return list.size();
}

QHash<int, QByteArray> SourceModel::roleNames() const {
  return {
      {Roles::SourceType, "sourceType"}, {Roles::Name, "name"}, {Roles::Subtitle, "subtitle"}, {Roles::Icon, "icon"}};
}

QVariant SourceModel::data(const QModelIndex& index, int role) const {
  if (list.empty()) {
    return "";
  }

  const auto it = std::next(list.begin(), index.row());

  switch (role) {
    case Roles::SourceType: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          value = "camera";

          break;
        }
        case MediaFile: {
          value = "media_file";

          break;
        }
        case Microphone: {
          value = "microphone";

          break;
        }
      }

      return value;
    }
    case Roles::Name: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          value = dynamic_cast<const CameraSource*>(it->get())->device.description();

          break;
        }
        case MediaFile: {
          value = dynamic_cast<const MediaFileSource*>(it->get())->url.fileName();

          break;
        }
        case Microphone: {
          value = dynamic_cast<const MicSource*>(it->get())->device.description();

          break;
        }
      }

      return value;
    }
    case Roles::Subtitle: {
      QString value;

      switch (it->get()->source_type) {
        case Camera: {
          auto format = dynamic_cast<const CameraSource*>(it->get())->format;

          if (!format.isNull()) {
            auto resolution = util::to_string(format.resolution().width()) + "x" +
                              util::to_string(format.resolution().height()) + "  " +
                              util::to_string(format.maxFrameRate()) + " fps";

            value = QString::fromStdString(resolution);
          }

          break;
        }
        case MediaFile: {
          auto file_size_mb = dynamic_cast<const MediaFileSource*>(it->get())->file_size_mb;
          auto duration = dynamic_cast<const MediaFileSource*>(it->get())->duration;
          auto frame_rate = dynamic_cast<const MediaFileSource*>(it->get())->frame_rate;
          auto audio_rate = dynamic_cast<const MediaFileSource*>(it->get())->audio_rate;

          value = frame_rate + " fps" + ", " + file_size_mb + " MiB" + ", " + duration + ", " + audio_rate;

          break;
        }
        case Microphone: {
          auto device = dynamic_cast<const MicSource*>(it->get())->device;

          std::string format;

          switch (device.preferredFormat().sampleFormat()) {
            case QAudioFormat::Unknown:
              format = "";
              break;
            case QAudioFormat::UInt8:
              format = "UInt8";
              break;
            case QAudioFormat::Int16:
              format = "Int16";
              break;
            case QAudioFormat::Int32:
              format = "Int32";
              break;
            case QAudioFormat::Float:
              format = "Float";
              break;
            case QAudioFormat::NSampleFormats:
              format = "NSampleFormats";
              break;
          }

          value = QString::fromStdString(std::format("{0:d} Hz, {1:d} {2}, {3}", device.preferredFormat().sampleRate(),
                                                     device.preferredFormat().channelCount(),
                                                     i18n("channels").toStdString(), format));

          break;
        }
      }

      return value;
    }
    case Roles::Icon: {
      QString name;

      switch (it->get()->source_type) {
        case Camera: {
          name = "camera-web-symbolic";

          break;
        }
        case MediaFile: {
          name = "video-symbolic";

          break;
        }
        case Microphone: {
          name = "audio-input-microphone-symbolic";

          break;
        }
      }

      return name;
    }
    default:
      return {};
  }
}

auto SourceModel::getList() -> QList<std::shared_ptr<Source>> {
  return list;
}

auto SourceModel::get_source(const int& rowIndex) -> std::shared_ptr<Source> {
  return list[rowIndex];
}

void SourceModel::append(std::shared_ptr<Source> source) {
  int pos = list.empty() ? 0 : list.size() - 1;

  beginInsertRows(QModelIndex(), pos, pos);

  switch (source->source_type) {
    case Camera:
      list.insert(0, source);
      break;
    case MediaFile:
      list.append(source);
      break;
    case Microphone:
      auto mic = dynamic_cast<const MicSource*>(source.get())->device;

      if (mic.isDefault()) {
        list.insert(0, source);

      } else {
        list.append(source);
      }
      break;
  }

  endInsertRows();

  emit dataChanged(index(0), index(list.size() - 1));
}

void SourceModel::reset() {
  beginResetModel();

  list.clear();

  endResetModel();
}

void SourceModel::removeSource(const int& rowIndex) {
  beginRemoveRows(QModelIndex(), rowIndex, rowIndex);

  list.remove(rowIndex);

  endRemoveRows();

  emit dataChanged(index(0), index(list.size() - 1));
}
