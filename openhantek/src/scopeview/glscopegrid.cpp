#include "glscopegrid.h"
#include "viewconstants.h"
#include "viewsettings.h"

#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>

using namespace Qt3DRender;
using namespace Qt3DExtras;
using namespace Qt3DCore;

GlScopeGrid::GlScopeGrid(const Settings::Colors *colors, Qt3DRender::QLayer *layer, QEntity *parent)
    : QEntity(parent), m_colors(colors), m_layer(layer) {
    createSubDivDots();
    createSubDivLines();
    createAxes();
    createBorder();
}

void GlScopeGrid::createSubDivDots() {
    auto lines = new Qt3DCore::QEntity();
    if (m_layer) lines->addComponent(m_layer);
    // material
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(lines);
    material->setAmbient(m_colors->grid());
    QObject::connect(m_colors->observer(), &Observer::changed, material,
                     [this, material]() { material->setAmbient(m_colors->grid()); });
    lines->addComponent(material);
    // buffer
    auto dataBuffer = new QBuffer(QBuffer::VertexBuffer, this);
    auto dataBufferNormals = new QBuffer(QBuffer::VertexBuffer, this);

    unsigned elements = 0;
    constexpr int subDotSize = 4 * int(DIVS_TIME / 2 - 1) * int(DIVS_VOLTAGE / 2 * DIVS_SUB - 1) +
                               4 * int(DIVS_VOLTAGE / 2 - 1) * int(DIVS_TIME / 2 * DIVS_SUB - 1);

    QByteArray vertextData;
    vertextData.resize(sizeof(QVector3D) * subDotSize);
    QVector3D *vertextDataF = reinterpret_cast<QVector3D *>(vertextData.data());

    QVector3D n(0.0f, 0.0f, 1.0f);
    QByteArray normalData;
    normalData.resize(sizeof(QVector3D) * subDotSize);
    QVector3D *normalDataF = reinterpret_cast<QVector3D *>(normalData.data());

    // Draw vertical dot lines
    for (int div = 1; div < DIVS_TIME / 2; ++div) {
        for (int dot = 1; dot < DIVS_VOLTAGE / 2 * DIVS_SUB; ++dot) {
            float dotPosition = (float)dot / DIVS_SUB;
            elements += 4;
            *(vertextDataF++) = QVector3D(-div, -dotPosition, -0.2f);
            *(vertextDataF++) = QVector3D(-div, dotPosition, -0.2f);
            *(vertextDataF++) = QVector3D(div, -dotPosition, -0.2f);
            *(vertextDataF++) = QVector3D(div, dotPosition, -0.2f);

            *(normalDataF++) = n;
            *(normalDataF++) = n;
            *(normalDataF++) = n;
            *(normalDataF++) = n;
        }
    }
    // Draw horizontal dot lines
    for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
        for (int dot = 1; dot < DIVS_TIME / 2 * DIVS_SUB; ++dot) {
            if (dot % DIVS_SUB == 0) continue; // Already done by vertical lines
            float dotPosition = (float)dot / DIVS_SUB;
            elements += 4;
            *(vertextDataF++) = (QVector3D(-dotPosition, -div, -0.2f));
            *(vertextDataF++) = (QVector3D(dotPosition, -div, -0.2f));
            *(vertextDataF++) = (QVector3D(-dotPosition, div, -0.2f));
            *(vertextDataF++) = (QVector3D(dotPosition, div, -0.2f));

            *(normalDataF++) = n;
            *(normalDataF++) = n;
            *(normalDataF++) = n;
            *(normalDataF++) = n;
        }
    }
    vertextData.resize(int(elements * sizeof(QVector3D)));
    normalData.resize(int(elements * sizeof(QVector3D)));
    dataBuffer->setData(vertextData);
    dataBufferNormals->setData(normalData);
    auto attr = new QAttribute(dataBuffer, QAttribute::defaultPositionAttributeName(), QAttribute::Float, 3, elements);
    auto attrNorm =
        new QAttribute(dataBufferNormals, QAttribute::defaultNormalAttributeName(), QAttribute::Float, 3, elements);
    auto mesh = new QGeometryRenderer(lines);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrNorm);
    mesh->setVertexCount((int)elements);
    mesh->setPrimitiveType(QGeometryRenderer::Points);
    mesh->setGeometry(geometry);
    lines->addComponent(mesh);
    lines->setParent(this);
}

void GlScopeGrid::createSubDivLines() {

    Qt3DExtras::QPhongMaterial *materialAxes = new Qt3DExtras::QPhongMaterial(this);
    materialAxes->setAmbient(m_colors->axes());
    QObject::connect(m_colors->observer(), &Observer::changed, materialAxes,
                     [this, materialAxes]() { materialAxes->setAmbient(m_colors->axes()); });

    QByteArray geomArray;
    const int numV = (int(DIVS_TIME * DIVS_SUB) - 1 + int(DIVS_VOLTAGE * DIVS_SUB) - 1) * 2;
    geomArray.resize(numV * sizeof(QVector3D));
    QVector3D *lineVp = reinterpret_cast<QVector3D *>(geomArray.data());

    QByteArray geomArrayN;
    geomArrayN.resize(numV * sizeof(QVector3D));
    QVector3D *lineNp = reinterpret_cast<QVector3D *>(geomArrayN.data());

    // Subdiv lines on horizontal axis
    for (int lineIndex = int(-DIVS_TIME / 2 * DIVS_SUB); lineIndex < int(DIVS_TIME / 2 * DIVS_SUB); ++lineIndex) {
        if (lineIndex == 0) continue;
        *(lineVp++) = QVector3D((float)lineIndex / DIVS_SUB, -0.1f, 0);
        *(lineVp++) = QVector3D((float)lineIndex / DIVS_SUB, +0.1f, 0);
        *(lineNp++) = QVector3D(0, 0, 1);
        *(lineNp++) = QVector3D(0, 0, 1);
    }
    // Subdiv lines on vertical axis
    for (int lineIndex = int(-DIVS_VOLTAGE / 2 * DIVS_SUB); lineIndex < int(DIVS_VOLTAGE / 2 * DIVS_SUB); ++lineIndex) {
        if (lineIndex == 0) continue;
        *(lineVp++) = QVector3D(-0.1f, (float)lineIndex / DIVS_SUB, 0);
        *(lineVp++) = QVector3D(+0.1f, (float)lineIndex / DIVS_SUB, 0);
        *(lineNp++) = QVector3D(0, 0, 1);
        *(lineNp++) = QVector3D(0, 0, 1);
    }

    auto sublines = new Qt3DCore::QEntity();
    if (m_layer) sublines->addComponent(m_layer);

    QBuffer *dataBuffer = new QBuffer(QBuffer::VertexBuffer, sublines);
    dataBuffer->setData(geomArray);
    QBuffer *dataBufferN = new QBuffer(QBuffer::VertexBuffer, sublines);
    dataBufferN->setData(geomArrayN);

    const auto namePosition = QAttribute::defaultPositionAttributeName();
    auto attr = new QAttribute(dataBuffer, namePosition, QAttribute::Float, 3, numV);
    const auto nameNormal = QAttribute::defaultNormalAttributeName();
    auto attrN = new QAttribute(dataBufferN, nameNormal, QAttribute::Float, 3, numV);
    auto mesh = new QGeometryRenderer(sublines);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrN);
    mesh->setVertexCount((int)numV);
    mesh->setPrimitiveType(QGeometryRenderer::Lines);
    mesh->setGeometry(geometry);

    sublines->addComponent(mesh);
    sublines->addComponent(materialAxes);
    sublines->setParent(this);
}

void GlScopeGrid::createAxes() {
    Qt3DExtras::QPhongMaterial *materialAxes = new Qt3DExtras::QPhongMaterial(this);
    materialAxes->setAmbient(m_colors->axes());
    QObject::connect(m_colors->observer(), &Observer::changed, materialAxes,
                     [this, materialAxes]() { materialAxes->setAmbient(m_colors->axes()); });

    const int numV = 4;
    const float frameV[] = {-0.5, 0,    0, +0.5, 0,    0,                    // horz
                            0,    -0.5, 0, 0,    +0.5, 0,                    // vert
                            0,    0,    1, 0,    0,    1, 0, 0, 1, 0, 0, 1}; // normales

    auto sublines = new Qt3DCore::QEntity();
    if (m_layer) sublines->addComponent(m_layer);

    QByteArray geomArray;
    geomArray.setRawData((char *)frameV, sizeof(frameV));
    QBuffer *dataBuffer = new QBuffer(QBuffer::VertexBuffer, sublines);
    dataBuffer->setData(geomArray);

    const auto namePosition = QAttribute::defaultPositionAttributeName();
    auto attr = new QAttribute(dataBuffer, namePosition, QAttribute::Float, 3, numV, 0);
    const auto nameNormal = QAttribute::defaultNormalAttributeName();
    auto attrN = new QAttribute(dataBuffer, nameNormal, QAttribute::Float, 3, numV, numV * sizeof(QVector3D));
    auto mesh = new QGeometryRenderer(sublines);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrN);
    mesh->setVertexCount((int)numV);
    mesh->setPrimitiveType(QGeometryRenderer::Lines);
    mesh->setGeometry(geometry);

    auto transform = new Qt3DCore::QTransform(sublines);
    transform->setScale3D(QVector3D(DIVS_TIME, DIVS_VOLTAGE, 0));

    sublines->addComponent(transform);
    sublines->addComponent(mesh);
    sublines->addComponent(materialAxes);
    sublines->setParent(this);
}

void GlScopeGrid::createBorder() {
    auto line = new Qt3DCore::QEntity();
    if (m_layer) line->addComponent(m_layer);

    Qt3DExtras::QPhongMaterial *materialBorder = new Qt3DExtras::QPhongMaterial(line);
    materialBorder->setAmbient(m_colors->border());
    QObject::connect(m_colors->observer(), &Observer::changed, materialBorder,
                     [this, materialBorder]() { materialBorder->setAmbient(m_colors->border()); });

    const int numV = 8;
    const float frameV[] = {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0,  // lines
                            1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0,  // lines
                            0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,  // normales
                            0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}; // normales

    QByteArray geomArray;
    geomArray.setRawData((char *)frameV, sizeof(frameV));
    QBuffer *dataBuffer = new QBuffer(QBuffer::VertexBuffer, line);
    dataBuffer->setData(geomArray);

    const auto namePosition = QAttribute::defaultPositionAttributeName();
    auto attr = new QAttribute(dataBuffer, namePosition, QAttribute::Float, 3, numV, 0, 0);
    const auto nameNormal = QAttribute::defaultNormalAttributeName();
    auto attrN = new QAttribute(dataBuffer, nameNormal, QAttribute::Float, 3, numV, numV * sizeof(QVector3D));
    auto mesh = new QGeometryRenderer(line);
    auto geometry = new QGeometry(mesh);
    geometry->addAttribute(attr);
    geometry->addAttribute(attrN);
    mesh->setVertexCount((int)numV);
    mesh->setPrimitiveType(QGeometryRenderer::Lines);
    mesh->setGeometry(geometry);

    auto transform = new Qt3DCore::QTransform(line);
    transform->setTranslation(QVector3D(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, 0));
    transform->setScale3D(QVector3D(DIVS_TIME, DIVS_VOLTAGE, 0));

    line->addComponent(mesh);
    line->addComponent(materialBorder);
    line->addComponent(transform);
    line->setParent(this);
}
