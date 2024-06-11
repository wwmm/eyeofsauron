#pragma once

#include <qaudioformat.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <QIODevice>
#include <QList>
#include <QPointF>
#include <vector>

namespace sound {

class IODevice : public QIODevice {
  Q_OBJECT
 public:
  explicit IODevice(QObject* parent = nullptr);

  void set_format(QAudioFormat device_format);

  static const int sampleCount = 2000;

 signals:
  void bufferChanged(std::vector<double> value);

 protected:
  qint64 readData(char* data, qint64 maxSize) override;
  qint64 writeData(const char* data, qint64 maxSize) override;

 private:
  QAudioFormat format;

  QList<QPointF> m_buffer;

  std::vector<double> buffer;
};

}  // namespace sound