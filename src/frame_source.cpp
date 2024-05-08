#include "frame_source.hpp"

Source::Source(SourceType source_type) : source_type(source_type) {}

CameraSource::CameraSource(QCameraDevice dev, QCameraFormat fmt)
    : Source(SourceType::Camera), device(dev), format(fmt) {}

VideoFileSource::VideoFileSource(QUrl file_url) : Source(SourceType::VideoFile), url(std::move(file_url)) {}