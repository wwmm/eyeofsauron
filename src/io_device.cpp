#include "io_device.hpp"
#include <qaudioformat.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtpreprocessorsupport.h>
#include <qtypes.h>
#include <algorithm>
#include <bit>
#include <cstddef>
#include <span>

namespace sound {

IODevice::IODevice(QObject* parent) : QIODevice(parent) {}

void IODevice::set_format(QAudioFormat device_format) {
  format = device_format;
}

qint64 IODevice::readData(char* data, qint64 maxSize) {
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return -1;
}

qint64 IODevice::writeData(const char* data, qint64 maxSize) {
  const int n_samples = maxSize / format.bytesPerSample();

  if (buffer.size() != static_cast<size_t>(n_samples)) {
    buffer.resize(n_samples);
  }

  auto input_data = std::span<const float>(std::bit_cast<const float*>(data), n_samples);

  std::copy(input_data.begin(), input_data.end(), buffer.begin());

  Q_EMIT bufferChanged(buffer);

  return maxSize;
}

}  // namespace sound