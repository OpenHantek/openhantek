#pragma once

#include <QColor>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QLayer>

namespace Settings {
struct Colors;
}

class GlScopeGrid : public Qt3DCore::QEntity {
    Q_OBJECT
  public:
    GlScopeGrid(const Settings::Colors *colors, Qt3DRender::QLayer *layer = nullptr, Qt3DCore::QEntity *parent = nullptr);

  private:
    const Settings::Colors *m_colors;
    Qt3DRender::QLayer *m_layer;
    void createSubDivDots();
    void createSubDivLines();
    void createAxes();
    void createBorder();
};
