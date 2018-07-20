#include "glscopegraph.h"
#include "viewsettings.h"

#include <QLayer>

#include <QDebug>

using namespace Qt3DRender;
using namespace Qt3DExtras;
using namespace Qt3DCore;
using namespace Settings;

GlScopeGraph::GlScopeGraph(QEntity *rootScene, const Colors *colors, const View *view, QLayer *layer)
    : m_layer(layer), m_colors(colors), m_view(view) {
    if (layer) addComponent(layer);
    setParent(rootScene);
    connect(colors->observer(), &Observer::changed, this, &GlScopeGraph::applyColors);
}

GlScopeGraph::~GlScopeGraph() {
    while (graphs.size()) graphs.pop_back();
}

void GlScopeGraph::writeData(PPresult *data) {
    // This implemention tries to minimize allocations especially on consecutive writes of this GlSopeGraph.
    // Spectrum and voltage channel graphs are generated and accessible for other writes via a linked
    // list. The list is expanded and shrunk if necessary. The semantic of an entry is not fixed.
    // If the user disables for instance a voltage channel that is inbetween other entries, then a spectrum
    // channel may overwrite what was a voltage graph before.
    auto it = graphs.begin();
    for (DataChannel &dataChannel : *data) {
        if (it == graphs.end())
            it = graphs.insert(it, new ChannelDetail(dataChannel.voltage.graph, m_layer, m_view, this));
        (*it)->channelID = dataChannel.channelID;
        (*it)->isSpectrum = false;
        (*it)->updateGraph(dataChannel.voltage.graph);
        ++it;

        if (it == graphs.end())
            it = graphs.insert(it, new ChannelDetail(dataChannel.spectrum.graph, m_layer, m_view, this));
        (*it)->channelID = dataChannel.channelID;
        (*it)->isSpectrum = true;
        (*it)->updateGraph(dataChannel.spectrum.graph);
        ++it;
    }
    while (it != graphs.end()) {
        delete *it;
        it = graphs.erase(it);
    }

    applyColors();
}

void GlScopeGraph::setColorAlpha(float alpha) {
    m_alpha = alpha;
    applyColors();
}

// inline QColor applyAlpha(const QColor &c, float alpha) {
//    return QColor::fromRgbF(c.redF(), c.greenF(), c.blueF(), std::min((float)c.alphaF(), alpha));
//}

void GlScopeGraph::applyColors(const Observer *) {
    for (ChannelDetail *c : graphs) {
        c->material->setAmbient(c->isSpectrum ? m_colors->spectrum(c->channelID) : m_colors->voltage(c->channelID));
        c->material->setAlpha(m_alpha);
    }
}

void GlScopeGraph::ChannelDetail::updateGraph(const ChannelGraph &channelData) {
    if (!channelData.size()) {
        setEnabled(false);
        return;
    }

    if ((unsigned)tempBuffer.size() < channelData.size() * sizeof(QVector3D)) {
        tempBuffer.resize((int)channelData.size() * (int)sizeof(QVector3D));
        memcpy(tempBuffer.data(), (char *)channelData.data(), channelData.size() * sizeof(QVector3D));
        dataBuffer->setData(tempBuffer);
    } else {
        // QByteArray b;b.resize(channelData.size() * sizeof(QVector3D));
        memcpy(tempBuffer.data(), (char *)channelData.data(), channelData.size() * sizeof(QVector3D));
        dataBuffer->setData(tempBuffer);
    }

    /// It looks like you need to set this each time you change the buffer contents.
    const unsigned s = (unsigned)channelData.size();
    mesh->setVertexCount((int)s);
    attr->setCount(s);

    setEnabled(true);
}

GlScopeGraph::ChannelDetail::ChannelDetail(const ChannelGraph &channelData, QLayer *layer, const View *view,
                                           QEntity *parent) {
    if (layer) addComponent(layer);

    // Data buffer
    dataBuffer->setUsage(Qt3DRender::QBuffer::DynamicDraw);

    // Mesh
    attr = new QAttribute(dataBuffer, QAttribute::defaultPositionAttributeName(), QAttribute::Float, 3, 0);
    mesh = new QGeometryRenderer(this);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    mesh->setVertexCount(0);

    // Set the interpolation type (line or points). React to settings changes.
    auto iChangedFun = [this](const View *view) {
        switch (view->interpolation()) {
        case DsoE::InterpolationMode::OFF:
            mesh->setPrimitiveType(QGeometryRenderer::Points);
            break;
        case DsoE::InterpolationMode::LINEAR:
            mesh->setPrimitiveType(QGeometryRenderer::LineStrip);
            break;
        case DsoE::InterpolationMode::SINC:
            mesh->setPrimitiveType(QGeometryRenderer::LineStrip);
            break;
        }
    };
    connect(view, &View::interpolationChanged, mesh, iChangedFun);
    iChangedFun(view);

    mesh->setGeometry(geometry);
    addComponent(mesh);
    addComponent(material);
    setParent(parent);
}
