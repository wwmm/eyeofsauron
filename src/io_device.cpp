#include "io_device.hpp"
#include <qobject.h>
#include <qtpreprocessorsupport.h>
#include <qtypes.h>

namespace sound {

IODevice::IODevice(QObject* parent) : QIODevice(parent) {}

qint64 IODevice::readData(char* data, qint64 maxSize) {
  Q_UNUSED(data);
  Q_UNUSED(maxSize);
  return -1;
}

qint64 IODevice::writeData(const char* data, qint64 maxSize) {
  static const int resolution = 4;

  if (m_buffer.isEmpty()) {
    m_buffer.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i) {
      m_buffer.append(QPointF(i, 0));
    }
  }

  int start = 0;

  const int availableSamples = int(maxSize) / resolution;

  if (availableSamples < sampleCount) {
    start = sampleCount - availableSamples;

    for (int s = 0; s < start; ++s) {
      m_buffer[s].setY(m_buffer.at(s + availableSamples).y());
    }
  }

  for (int s = start; s < sampleCount; ++s, data += resolution) {
    m_buffer[s].setY(qreal(uchar(*data) - 128) / qreal(128));
  }

  //   m_series->replace(m_buffer);

  return (sampleCount - start) * resolution;
}

}  // namespace sound