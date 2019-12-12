#include "DepthMapEntity.hpp"
#include "mv_point3d.hpp"
#include "mv_point2d.hpp"
#include "mv_matrix3x3.hpp"

#include "../jetColorMap.hpp"

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>

#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DCore/QTransform>

#include <QDebug>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <iostream>

namespace oiio = OIIO;


namespace depthMapEntity {


struct Vec3f
{
    Vec3f() {}
    Vec3f(float x_, float y_, float z_)
      : x(x_)
      , y(y_)
      , z(z_)
    {}
    union {
        struct
        {
            float x, y, z;
        };
        float m[3];
    };

    inline Vec3f operator-(const Vec3f& p) const
    {
        return Vec3f(x - p.x, y - p.y, z - p.z);
    }

    inline double size() const
    {
        double d = x * x + y * y + z * z;
        if(d == 0.0)
        {
            return 0.0;
        }

        return sqrt(d);
    }
};

inline Vec3f cross(const Vec3f& a, const Vec3f& b)
{
    Vec3f vc;
    vc.x = a.y * b.z - a.z * b.y;
    vc.y = a.z * b.x - a.x * b.z;
    vc.z = a.x * b.y - a.y * b.x;

    return vc;
}

DepthMapEntity::DepthMapEntity(Qt3DCore::QNode* parent)
    : Qt3DCore::QEntity(parent)
    , _displayMode(DisplayMode::Unknown)
    , _pointSizeParameter(new Qt3DRender::QParameter)
{
    qDebug() << "[DepthMapEntity] DepthMapEntity";
    createMaterials();
}

void DepthMapEntity::setSource(const QUrl& value)
{
    if(_source == value)
        return;
    _source = value;
    loadDepthMap();
    Q_EMIT sourceChanged();
}

void DepthMapEntity::setDisplayMode(const DepthMapEntity::DisplayMode& value)
{
    if(_displayMode == value)
        return;
    _displayMode = value;
    updateMaterial();

    Q_EMIT displayModeChanged();
}

void DepthMapEntity::setDisplayColor(bool value)
{
    if(_displayColor == value)
        return;
    _displayColor = value;
    updateMaterial();

    Q_EMIT displayColorChanged();
}

void DepthMapEntity::updateMaterial()
{
    if(_status != DepthMapEntity::Ready)
      return;

    Qt3DRender::QMaterial* newMaterial = nullptr;

    switch(_displayMode)
    {
    case DisplayMode::Points:
        newMaterial = _cloudMaterial;
        _meshRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
        break;
    case DisplayMode::Triangles:
        _meshRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
        if(_displayColor) 
            newMaterial = _colorMaterial;
        else 
            newMaterial = _diffuseMaterial;
        break;
    default:
        newMaterial = _diffuseMaterial;
    }

    if(newMaterial == _currentMaterial)
        return;

    if(_currentMaterial)
        removeComponent(_currentMaterial);

    _currentMaterial = newMaterial;
    addComponent(_currentMaterial);
}

void DepthMapEntity::setPointSize(const float& value)
{
    if(_pointSize == value)
        return;
    _pointSize = value;
    _pointSizeParameter->setValue(value);
    _cloudMaterial->setEnabled(_pointSize > 0.0f);
    Q_EMIT pointSizeChanged();
}

// private
void DepthMapEntity::createMaterials()
{
    using namespace Qt3DRender;
    using namespace Qt3DExtras;

    {
        _cloudMaterial = new QMaterial(this);

        // configure cloud material
        QEffect* effect = new QEffect;
        QTechnique* technique = new QTechnique;
        QRenderPass* renderPass = new QRenderPass;
        QShaderProgram* shaderProgram = new QShaderProgram;

        // set vertex shader
        shaderProgram->setVertexShaderCode(R"(#version 130
        in vec3 vertexPosition;
        in vec3 vertexColor;
        out vec3 color;
        uniform mat4 mvp;
        uniform mat4 projectionMatrix;
        uniform mat4 viewportMatrix;
        uniform float pointSize;
        void main()
        {
            color = vertexColor;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
            gl_PointSize = max(viewportMatrix[1][1] * projectionMatrix[1][1] * pointSize / gl_Position.w, 1.0);
        }
        )");

        // set fragment shader
        shaderProgram->setFragmentShaderCode(R"(#version 130
            in vec3 color;
            out vec4 fragColor;
            void main(void)
            {
                fragColor = vec4(color, 1.0);
            }
        )");

        // add a pointSize uniform
        _pointSizeParameter->setName("pointSize");
        _pointSizeParameter->setValue(_pointSize);
        _cloudMaterial->addParameter(_pointSizeParameter);

        // build the material
        renderPass->setShaderProgram(shaderProgram);
        technique->addRenderPass(renderPass);
        effect->addTechnique(technique);
        _cloudMaterial->setEffect(effect);
    }
    {
        _colorMaterial = new QPerVertexColorMaterial(this);
    }
    {
        _diffuseMaterial = new QDiffuseSpecularMaterial(this);
        _diffuseMaterial->setAmbient(QColor(0, 0, 0));
        _diffuseMaterial->setDiffuse(QColor(255, 255, 255));
        _diffuseMaterial->setSpecular(QColor(0, 0, 0));
        _diffuseMaterial->setShininess(0.0);
    }
}

bool validTriangleRatio(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    std::vector<double> distances = {
        (a - b).size(),
        (b - c).size(),
        (c - a).size()
    };
    double mi = std::min({distances[0], distances[1], distances[2]});
    double ma = std::max({distances[0], distances[1], distances[2]});
    if(ma == 0.0)
        return false;
    return (mi / ma) > 1.0 / 5.0;
}


// private
void DepthMapEntity::loadDepthMap()
{
    _status = DepthMapEntity::Loading;

    if(_meshRenderer)
    {
        removeComponent(_meshRenderer);
        _meshRenderer->deleteLater();
        _meshRenderer = nullptr;
    }

    qDebug() << "[DepthMapEntity] loadDepthMap";
    if(!_source.isValid())
    {
        _status = DepthMapEntity::Error;
        return;
    }

    qDebug() << "[DepthMapEntity] Load Depth Map: " << _source.toLocalFile();

    // verify that the file is a valid depthMap based on its metadata
    {
        const std::string imagePath = _source.toLocalFile().toStdString();
        std::unique_ptr<oiio::ImageInput> in(oiio::ImageInput::open(imagePath));
        const oiio::ImageSpec& inSpec = in->spec();
        // check for a specific metadata entry
        const oiio::ParamValue* param = inSpec.find_attribute("AliceVision:CArr");
        if(!param)
        {
            _status = DepthMapEntity::Error;
            return;
        }
    }

    using namespace Qt3DRender;

    oiio::ImageSpec configSpec;
    // libRAW configuration
    configSpec.attribute("raw:auto_bright", 0);       // don't want exposure correction
    configSpec.attribute("raw:use_camera_wb", 1);     // want white balance correction
    configSpec.attribute("raw:ColorSpace", "sRGB");   // want colorspace sRGB
    configSpec.attribute("raw:use_camera_matrix", 3); // want to use embeded color profile

    oiio::ImageBuf inBuf(_source.toLocalFile().toStdString(), 0, 0, NULL, &configSpec);
    const oiio::ImageSpec& inSpec = inBuf.spec();

    qDebug() << "[DepthMapEntity] Image Size: " << inSpec.width << "x" << inSpec.height;

    point3d CArr;
    const oiio::ParamValue * cParam = inSpec.find_attribute("AliceVision:CArr"); // , oiio::TypeDesc(oiio::TypeDesc::DOUBLE, oiio::TypeDesc::VEC3));
    if(cParam)
    {
        qDebug() << "[DepthMapEntity] CArr: " << cParam->nvalues();
        std::copy_n((const double*)cParam->data(), 3, CArr.m);
    }
    else
    {
        qDebug() << "[DepthMapEntity] missing metadata CArr.";
    }

    matrix3x3 iCamArr;
    const oiio::ParamValue * icParam = inSpec.find_attribute("AliceVision:iCamArr", oiio::TypeDesc(oiio::TypeDesc::DOUBLE, oiio::TypeDesc::MATRIX33));
    if(icParam)
    {
        qDebug() << "[DepthMapEntity] iCamArr: " << icParam->nvalues();
        std::copy_n((const double*)icParam->data(), 9, iCamArr.m);
    }
    else
    {
        qDebug() << "[DepthMapEntity] missing metadata iCamArr.";
    }

    const QUrl simPath = QUrl::fromLocalFile(_source.path().replace("depthMap", "simMap"));
    oiio::ImageBuf simBuf;

    if(simPath.isValid())
    {
        qDebug() << "[DepthMapEntity] Load Sim Map: " << simPath;
        simBuf.reset(simPath.toLocalFile().toStdString(), 0, 0, NULL, &configSpec);
    }

    const oiio::ImageSpec& simSpec = simBuf.spec();
    const bool validSimMap = (simSpec.width == inSpec.width) && (simSpec.height == inSpec.height);

    oiio::ImageBufAlgo::PixelStats stats;
    oiio::ImageBufAlgo::computePixelStats(stats, inBuf);

    std::vector<int> indexPerPixel(inSpec.width * inSpec.height, -1);
    std::vector<Vec3f> positions;
    std::vector<Color32f> colors;

    for(int y = 0; y < inSpec.height; ++y)
    {
        for(int x = 0; x < inSpec.width; ++x)
        {
            float depthValue = 0.0f;
            inBuf.getpixel(x, y, &depthValue, 1);
            if(depthValue == -1.f)
                continue;

            point3d p = CArr + (iCamArr * point2d((double)x, (double)y)).normalize() * depthValue;
            Vec3f position(p.x, p.y, p.z);

            indexPerPixel[y * inSpec.width + x] = positions.size();
            positions.push_back(position);

            if(validSimMap)
            {
                float simValue = 0.0f;
                simBuf.getpixel(x, y, &simValue, 1);
                Color32f color = getColor32fFromJetColorMapClamp(simValue);
                colors.push_back(color);
            }
            else
            {
                const float range = stats.max[0] - stats.min[0];
                float normalizedDepthValue = range != 0.0f ? (depthValue - stats.min[0]) / range : 1.0f;
                Color32f color = getColor32fFromJetColorMapClamp(normalizedDepthValue);
                colors.push_back(color);
            }
        }
    }

    qDebug() << "[DepthMapEntity] Valid Depth Values: " << positions.size();

    // create geometry
    QGeometry* customGeometry = new QGeometry;

    // vertices buffer
    std::vector<int> trianglesIndexes;
    trianglesIndexes.reserve(2*3*positions.size());
    for(int y = 0; y < inSpec.height-1; ++y)
    {
        for(int x = 0; x < inSpec.width-1; ++x)
        {
            int pixelIndexA = indexPerPixel[y * inSpec.width + x];
            int pixelIndexB = indexPerPixel[(y + 1) * inSpec.width + x];
            int pixelIndexC = indexPerPixel[(y + 1) * inSpec.width + x + 1];
            int pixelIndexD = indexPerPixel[y * inSpec.width + x + 1];
            if(pixelIndexA != -1 &&
                pixelIndexB != -1 &&
                pixelIndexC != -1 &&
                validTriangleRatio(positions[pixelIndexA], positions[pixelIndexB], positions[pixelIndexC]))
            {
                trianglesIndexes.push_back(pixelIndexA);
                trianglesIndexes.push_back(pixelIndexB);
                trianglesIndexes.push_back(pixelIndexC);
            }
            if(pixelIndexC != -1 &&
                pixelIndexD != -1 &&
                pixelIndexA != -1 &&
                validTriangleRatio(positions[pixelIndexC], positions[pixelIndexD], positions[pixelIndexA]))
            {
                trianglesIndexes.push_back(pixelIndexC);
                trianglesIndexes.push_back(pixelIndexD);
                trianglesIndexes.push_back(pixelIndexA);
            }
        }
    }
    qDebug() << "[DepthMapEntity] Nb triangles: " << trianglesIndexes.size();

    std::vector<Vec3f> triangles;
    triangles.resize(trianglesIndexes.size());
    for(int i = 0; i < trianglesIndexes.size(); ++i)
    {
        triangles[i] = positions[trianglesIndexes[i]];
    }
    std::vector<Vec3f> normals;
    normals.resize(triangles.size());
    for(int i = 0; i < trianglesIndexes.size(); i+=3)
    {
        Vec3f normal = cross(triangles[i+1]-triangles[i], triangles[i+2]-triangles[i]);
        for(int t = 0; t < 3; ++t)
            normals[i+t] = normal;
    }

    QBuffer* vertexBuffer = new QBuffer(QBuffer::VertexBuffer);
    QByteArray trianglesData((const char*)&triangles[0], triangles.size() * sizeof(Vec3f));
    vertexBuffer->setData(trianglesData);

    QBuffer* normalBuffer = new QBuffer(QBuffer::VertexBuffer);
    QByteArray normalsData((const char*)&normals[0], normals.size() * sizeof(Vec3f));
    normalBuffer->setData(normalsData);

    QAttribute* positionAttribute = new QAttribute(this);
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setDataType(QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(sizeof(Vec3f));
    positionAttribute->setCount(triangles.size());

    QAttribute* normalAttribute = new QAttribute(this);
    normalAttribute->setName(QAttribute::defaultNormalAttributeName());
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(normalBuffer);
    normalAttribute->setDataType(QAttribute::Float);
    normalAttribute->setDataSize(3);
    normalAttribute->setByteOffset(0);
    normalAttribute->setByteStride(sizeof(Vec3f));
    normalAttribute->setCount(normals.size());

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    // customGeometry->setBoundingVolumePositionAttribute(positionAttribute);
        
    // Duplicate colors as we cannot use indexes!
    std::vector<Color32f> colorsFlat;
    colorsFlat.reserve(trianglesIndexes.size());
    for(int i = 0; i < trianglesIndexes.size(); ++i)
    {
        colorsFlat.push_back(colors[trianglesIndexes[i]]);
    }

    // read color data
    QBuffer* colorDataBuffer = new QBuffer(QBuffer::VertexBuffer);
    QByteArray colorData((const char*)colorsFlat[0].m, colorsFlat.size() * 3 * sizeof(float));
    colorDataBuffer->setData(colorData);

    QAttribute* colorAttribute = new QAttribute;
    qDebug() << "Qt3DRender::QAttribute::defaultColorAttributeName(): " << Qt3DRender::QAttribute::defaultColorAttributeName();
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
    colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorDataBuffer);
    colorAttribute->setDataType(QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(colorsFlat.size());
    customGeometry->addAttribute(colorAttribute);

    // create the geometry renderer
    _meshRenderer = new QGeometryRenderer;
    _meshRenderer->setGeometry(customGeometry);
    
    _status = DepthMapEntity::Ready;
    
    // add components
    addComponent(_meshRenderer);
    updateMaterial();
    qDebug() << "DepthMapEntity: Mesh Renderer added.";
}

} // namespace
