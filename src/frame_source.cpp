#include "frame_source.hpp"
#include <qdebug.h>
#include <qlogging.h>
#include <cmath>
#include <format>
#include <string>
#include <utility>
#define _UNICODE
#include <MediaInfo/MediaInfo.h>
#include <MediaInfo/MediaInfo_Const.h>
#include <qcameradevice.h>
#include <qurl.h>
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

      util::str_to_num(f_size_str.toStdString(), f_size_bytes);

      file_size_mb = QString::fromStdString(std::format("{0:.1f}", f_size_bytes / 1024 / 1024));

      auto duration_str =
          QString::fromStdWString(MI.Get(Stream_General, 0, static_cast<String>(L"Duration"), Info_Text, Info_Name));

      util::str_to_num(duration_str.toStdString(), duration_ms);

      int minutes = std::floor(duration_ms / 60000);

      float seconds = duration_ms / 1000 - minutes * 60;

      duration = QString::fromStdString(std::format("{0:d}:{1:0>4.1f}", minutes, seconds));

      auto frame_rate_str =
          QString::fromStdWString(MI.Get(Stream_General, 0, static_cast<String>(L"FrameRate"), Info_Text, Info_Name));

      util::str_to_num(frame_rate_str.toStdString(), fps);

      frame_rate = QString::fromStdString(std::format("{0:.1f}", fps));

      MI.Close();
    } else {
      util::warning("failed to get media information");
    }
  }
}