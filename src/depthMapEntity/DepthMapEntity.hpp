#pragma once

#include <Qt3DCore/QEntity>
#include <QtCore/QUrl>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QMaterial>


namespace depthMapEntity {

class DepthMapEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_ENUMS(DisplayMode)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged);
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode NOTIFY displayModeChanged);

public:
    DepthMapEntity(Qt3DCore::QNode* = nullptr);
    ~DepthMapEntity() = default;

    enum class DisplayMode {
        Points,
        Triangles,
        Unknown
    };

public:
    Q_SLOT const QUrl& source() const { return _source; }
    Q_SLOT void setSource(const QUrl&);

    Q_SLOT DisplayMode displayMode() const { return _displayMode; }
    Q_SLOT void setDisplayMode(const DisplayMode&);

private:
    void loadDepthMap();
    void createMaterials();

public:
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void displayModeChanged();

private:
    QUrl _source;
    DisplayMode _displayMode;

    Qt3DRender::QMaterial* _cloudMaterial;
};

}
