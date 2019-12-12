#pragma once

#include <Qt3DCore/QEntity>
#include <QtCore/QUrl>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <QDiffuseSpecularMaterial>
#include <QGeometryRenderer>


namespace depthMapEntity {

class DepthMapEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_ENUMS(DisplayMode)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged);
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode NOTIFY displayModeChanged);
    Q_PROPERTY(bool displayColor READ displayColor WRITE setDisplayColor NOTIFY displayColorChanged);
    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged);

public:

    // Identical to SceneLoader.Status
    enum Status { 
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

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

    Status status() const { return _status; }

    void setStatus(Status status) { 
        if(status == _status) 
            return; 
        _status = status; 
        Q_EMIT statusChanged(_status); 
    }

    Q_SLOT DisplayMode displayMode() const { return _displayMode; }
    Q_SLOT void setDisplayMode(const DisplayMode&);

    Q_SLOT bool displayColor() const { return _displayColor; }
    Q_SLOT void setDisplayColor(bool);

    Q_SLOT float pointSize() const { return _pointSize; }
    Q_SLOT void setPointSize(const float& value);

private:
    void loadDepthMap();
    void createMaterials();
    void updateMaterial();

public:
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void statusChanged(Status status);
    Q_SIGNAL void displayModeChanged();
    Q_SIGNAL void displayColorChanged();
    Q_SIGNAL void pointSizeChanged();

private:
    Status _status = DepthMapEntity::None;
    QUrl _source;
    DisplayMode _displayMode = DisplayMode::Triangles;
    bool _displayColor = true;
    float _pointSize = 0.5f;
    Qt3DRender::QParameter* _pointSizeParameter;
    Qt3DRender::QMaterial* _cloudMaterial;
    Qt3DExtras::QDiffuseSpecularMaterial* _diffuseMaterial;
    Qt3DExtras::QPerVertexColorMaterial* _colorMaterial;
    Qt3DRender::QMaterial* _currentMaterial = nullptr;
    Qt3DRender::QGeometryRenderer* _meshRenderer = nullptr;
};

}
