#include "glscopegraph.h"
#include "viewsettings.h"
#include <QDebug>
#include <QImage>

Graph::Graph() : buffer(QOpenGLBuffer::VertexBuffer) {
    moveToThread(&thread);
    thread.start();
}

void Graph::draw() {
    if (!buffer.isCreated()) return;

    QWriteLocker locker(&lock);

    // Determine memory
    int neededMemory = 0;
    for (ChannelGraph &cg : data->vaChannelVoltage) neededMemory += cg.size() * sizeof(QVector3D);
    for (ChannelGraph &cg : data->vaChannelSpectrum) neededMemory += cg.size() * sizeof(QVector3D);

    context->makeCurrent(&offscreen);
    auto *gl = context->functions();
    fbo->bind();
    buffer.bind();
    program->bind();

    gl->glViewport(0, 0, size.width(), size.height());
    gl->glClearColor(0.12, 0.12, 0.12, 1.0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl->glLineWidth(1);

    // Allocate space if necessary
    if (neededMemory > allocatedMem) {
        buffer.allocate(neededMemory);
        allocatedMem = neededMemory;
    }

    const GLenum dMode = (view->interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP;

    // Write data to buffer
    int offset = 0;

    for (ChannelID channel = 0; channel < data->vaChannelVoltage.size(); ++channel) {
        int dataSize;

        ChannelGraph &gVoltage = data->vaChannelVoltage[channel];

        dataSize = int(gVoltage.size() * sizeof(QVector3D));
        buffer.write(offset, gVoltage.data(), dataSize);
        program->enableAttributeArray(vertexLocation);
        program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, 0);

        program->setUniformValue(colorLocation, view->screen.voltage[channel]);
        context->functions()->glDrawArrays(dMode, 0, (int)gVoltage.size());

        offset += dataSize;

        // Spectrum channel

        if (channel < vaoSpectrum.size()) {
            ChannelGraph &gSpectrum = data->vaChannelSpectrum[channel];
            dataSize = int(gSpectrum.size() * sizeof(QVector3D));
            buffer.write(offset, gSpectrum.data(), dataSize);
            program->enableAttributeArray(vertexLocation);
            program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, 0);

            program->setUniformValue(colorLocation, view->screen.spectrum[channel]);
            context->functions()->glDrawArrays(dMode, 0, (int)gSpectrum.size());

            offset += dataSize;
        }
    }

    buffer.release();

    // Multisampling supported. We need a final fbo to blit from the render target fbo,
    // to get an OpenGL texture.
    if (fbo->format().samples() > 0) {
        fboFinal->bind();
        QOpenGLFramebufferObject::blitFramebuffer(fboFinal, fbo);
    }
    ready = true;

    gl->glFlush();

    qWarning() << "normal" << fbo->width() << fbo->height() << fbo->format().samples();
    qWarning() << "final" << fboFinal->width() << fboFinal->height() << fboFinal->format().samples();
    fboFinal->toImage().save("/home/david/build-openhantek-clang-Debug/out.png");
    emit fboReady();
}

void Graph::createInThread() {
    QOpenGLContext *global = this->context;
    this->context = new QOpenGLContext(this);
    this->context->setFormat(global->format());
    this->context->setShareContext(global);
    if (!this->context->create()) {
        qWarning() << "Couldn't create context for scope graph";
        return;
    }
    this->context->makeCurrent(&offscreen);

    if (!buffer.create()) {
        qWarning() << "Couldn't create buffer for scope graph";
        return;
    }
    buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    format.setSamples(0); // TODO remove
    fbo = new QOpenGLFramebufferObject(size.width(), size.height(), format);
    if (fbo->format().samples() > 0) {
        // Multisampling supported. We need a final fbo to blit from the render target fbo,
        // to get an OpenGL texture.
        QOpenGLFramebufferObjectFormat format;
        format.setSamples(0);
        fboFinal = new QOpenGLFramebufferObject(size.width(), size.height(), format);
    } else {
        // No multisampling supported, the final fbo is the same as the render target fbo
        fboFinal = fbo;
    }
}

Graph::~Graph() {
    thread.quit();
    destroy();
}

void Graph::create(const QRect &size, QOpenGLShaderProgram *program, QOpenGLContext *context, DsoSettingsView *view,
                   int vertexLocation, int colorLocation) {
    this->program = program;
    this->vertexLocation = vertexLocation;
    this->colorLocation = colorLocation;
    this->view = view;
    this->size = size;
    this->context = context;
    offscreen.setFormat(context->surface()->format());
    offscreen.create();

    QMetaObject::invokeMethod(this, "createInThread", Qt::QueuedConnection);
}

void Graph::destroy() {
    delete fboFinal;
    if (fboFinal != fbo) delete fbo;
    fbo = nullptr;
    fboFinal = nullptr;

    if (offscreen.isValid()) offscreen.destroy();
    if (buffer.isCreated()) buffer.destroy();

    delete context;
}

void Graph::redraw() {
    if (!buffer.isCreated()) {
        qWarning() << "writeData executed on a Graph object that has not been initalized!";
        return;
    }
    QMetaObject::invokeMethod(this, "draw", Qt::QueuedConnection);
    ;
}
