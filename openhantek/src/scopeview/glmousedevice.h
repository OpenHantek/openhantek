#pragma once

#include <Qt3DInput/QMouseDevice>

/**
 * In Qt5.9+ we can use QObjectPicker. But for now we have a QMouseDevice
 * that also keeps track which 3D object is focused. There can only be one focused
 * object at a time. Objects can grab and release the focus. If an object with
 * a higher priority (QlIcon in contrast to QlFrame for instance) request the focus,
 * it will be stolen from the current focused object.
 *
 * GlIcon and GlFrame support this focus method.
 */
class GlMouseDevice : public Qt3DInput::QMouseDevice {
    Q_OBJECT
  public:
    GlMouseDevice();
    inline void unsetFocusObject(QObject *focusObject) {
        if (m_focusObject == focusObject) {
            disconnect(m_focusObject, &QObject::destroyed, this, &GlMouseDevice::unsetFocusObject);
            m_focusObject = nullptr;
            m_focusObjectPriority = 0;
            emit focusObjectChanged(m_focusObject);
        }
    }
    inline bool grabFocus(QObject *focusObject, int priority = 0) {
        if (m_focusObject == focusObject) return true;

        if (m_focusObject) {
            if (priority > m_focusObjectPriority) {
                unsetFocusObject(m_focusObject);
                emit focusStoolen();
                m_focusObjectPriority = priority;
                m_focusObject = focusObject;
                return true;
            } else
                return false;
        } else {
            m_focusObject = focusObject;
            connect(m_focusObject, &QObject::destroyed, this, &GlMouseDevice::unsetFocusObject);
            emit focusObjectChanged(m_focusObject);
            return true;
        }
    }

  private:
    QObject *m_focusObject = nullptr;
    int m_focusObjectPriority = 0;
  signals:
    void focusObjectChanged(void *m_focusObject);
    void focusStoolen();
};
