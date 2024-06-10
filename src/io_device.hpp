#pragma once

#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <QIODevice>
#include <QList>
#include <QPointF>

namespace sound {

class IODevice : public QIODevice {
  Q_OBJECT
 public:
  explicit IODevice(QObject* parent = nullptr);

  static const int sampleCount = 2000;

 protected:
  qint64 readData(char* data, qint64 maxSize) override;
  qint64 writeData(const char* data, qint64 maxSize) override;

 private:
  QList<QPointF> m_buffer;
};

}  // namespace sound