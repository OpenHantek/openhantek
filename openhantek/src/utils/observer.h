#pragma once

#include <QObject>
#include <QRectF>

/**
 * Helper class to compute scope coordinates from screen coordinates. It is usually
 * enough to have one instance, update it in your main window and pass it around.
 */
class Observer : public QObject {
    Q_OBJECT
  public:
    Observer(void *target) { m_target = target; }
    inline void update() { emit changed(this); }
    template <class T> inline T *get() { static_cast<T *>(m_target); }
  private:
    void *m_target;
  signals:
    void changed(const Observer *target);
};
