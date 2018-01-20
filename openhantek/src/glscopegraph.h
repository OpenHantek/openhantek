// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <memory>

#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QReadWriteLock>
#include <QThread>
#include <QtGlobal>

#include "post/ppresult.h"

struct DsoSettingsView;

/**
 * A graph object represents one sample snaptshot with all channels
 * visualized on an OpenGL offscreen surface.
 */
class Graph : public QObject {
    Q_OBJECT
  public:
    explicit Graph();
    /// Calls destroy() internally
    ~Graph();

    /**
     * Allocate necessary resources and assign required parameters
     * @param size
     * @param program
     * @param context
     * @param view
     * @param vertexLocation
     * @param colorLocation
     */
    void create(const QRect &size, QOpenGLShaderProgram *program, QOpenGLContext *context, DsoSettingsView *view,
                int vertexLocation, int colorLocation);

    /// Deallocate resources. Can be called multiple times.
    void destroy();

    /// Will queue a draw request to the worker thread queue.
    void redraw();

    /// Return true if the framebuffer is created and was drawn at least once
    inline bool isReady() { return ready; }

    /// Return the framebuffer
    inline QOpenGLFramebufferObject *getFBO() { return fboFinal; }

    /// Return the read/write lock of this graph. You should always aquire a read lock
    /// before operating on the framebuffer of this graph object.
    inline QReadWriteLock *getLock() { return &lock; }

    /// Assign new data to the graph. You need to call redraw() as well.
    inline void assignData(std::shared_ptr<PPresult> &data) { this->data = data; }

  public:
    Graph(const Graph &) = delete;
    Graph(const Graph &&) = delete;

  private slots:
    void createInThread();
    void draw();

  signals:
    void fboReady();

  private:
    // Threading
    QReadWriteLock lock;
    QThread thread;

    // Data
    std::shared_ptr<PPresult> data;
    QRect size;
    DsoSettingsView *view;

    // OpenGL buffers
    bool ready = false;
    int allocatedMem = 0;
    QOpenGLBuffer buffer;
    QOpenGLFramebufferObject *fbo;
    QOpenGLFramebufferObject *fboFinal;
    QOpenGLShaderProgram *program;
    QOpenGLContext *context;
    QOffscreenSurface offscreen;
    int vertexLocation;
    int colorLocation;
};
