#pragma once

#include "post/ppresult.h"
#include <memory>

#include <QColor>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QPhongAlphaMaterial>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QLayer>

namespace Settings {
struct Colors;
class View;
}
class Observer;

class GlScopeGraph : public Qt3DCore::QEntity {
    Q_OBJECT
  public:
    GlScopeGraph(Qt3DCore::QEntity *rootScene, const Settings::Colors *colors, const Settings::View *view,
                 Qt3DRender::QLayer *layer = nullptr);
    ~GlScopeGraph();
    GlScopeGraph(GlScopeGraph &&) = default;
    GlScopeGraph(const GlScopeGraph &) = delete;
    void writeData(PPresult *data);
    void setColorAlpha(float alpha);

  private:
    struct ChannelDetail : public Qt3DCore::QEntity {
        ChannelDetail(const ChannelGraph &channelData, Qt3DRender::QLayer *m_layer, const Settings::View *view,Qt3DCore::QEntity*parent);
        ChannelDetail(ChannelDetail &&) = default;
        ChannelDetail(const ChannelDetail &) = delete;
        void updateGraph(const ChannelGraph &channelData);
        Qt3DRender::QBuffer *dataBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, this);
        Qt3DExtras::QPhongAlphaMaterial *material = new Qt3DExtras::QPhongAlphaMaterial(this);
        Qt3DRender::QGeometryRenderer *mesh;
        Qt3DRender::QAttribute* attr;
        unsigned meshInit = 0;
        ChannelID channelID;
        bool isSpectrum = false;
        QByteArray tempBuffer;
    };

  private:
    std::list<ChannelDetail *> graphs;
    float m_alpha = 1.0;
    Qt3DRender::QLayer *m_layer;
    const Settings::Colors *m_colors;
    const Settings::View *m_view;

  private:
    void applyColors(const Observer *target = nullptr);
};
