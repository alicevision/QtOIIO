#include "DepthMapEntity.hpp"
#include "mv_point3d.hpp"
#include "mv_point2d.hpp"
#include "mv_matrix3x3.hpp"

#include "../jetColorMap.hpp"

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QObjectPicker>
// #include <Qt3DRender/QPickEvent>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QPhongMaterial>

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DCore/QTransform>

#include <QDebug>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <iostream>

namespace oiio = OIIO;

// #define QTOIIO_DEPTHMAP_USE_INDEXES 1

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


DepthMapEntity::DepthMapEntity(Qt3DCore::QNode* parent)
    : Qt3DCore::QEntity(parent)
    , _displayMode(DisplayMode::Unknown)
{
    std::cout << "[DepthMapEntity] DepthMapEntity" << std::endl;
    qDebug() << "[DepthMapEntity] DepthMapEntity";
    setDisplayMode(DisplayMode::Triangles);
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

    createMaterials();
    loadDepthMap();

    Q_EMIT displayModeChanged();
}

// private
void DepthMapEntity::createMaterials()
{
    using namespace Qt3DRender;
    using namespace Qt3DExtras;

    switch(_displayMode)
    {
    case DisplayMode::Points:
    {
        _cloudMaterial = new QMaterial(this);

        // configure cloud material
        QEffect* effect = new QEffect;
        QTechnique* technique = new QTechnique;
        QRenderPass* renderPass = new QRenderPass;
        QShaderProgram* shaderProgram = new QShaderProgram;

        // set vertex shader
        shaderProgram->setVertexShaderCode(R"(#version 330 core
            uniform mat4 modelViewProjection;
            in vec3 vertexPosition;
            in vec3 vertexColor;
            out vec3 colors;
            in float pixelSize;
            out float pixelSizes;
            void main(void)
            {
                gl_Position = modelViewProjection * vec4(vertexPosition, 1.0f);
                colors = vertexColor;
                pixelSizes = pixelSize;
            }
        )");

        // set fragment shader
        shaderProgram->setFragmentShaderCode(R"(#version 330 core
            in vec3 color;
            out vec4 fragColor;
            void main(void)
            {
                fragColor = vec4(color, 1.0);
            }
        )");

        // set geometry shader
        shaderProgram->setGeometryShaderCode(R"(#version 330
            layout(points) in;
            layout(triangle_strip) out;
            layout(max_vertices = 4) out;
            uniform mat4 projectionMatrix;
            in float pixelSizes[];
            in vec3 colors[];
            out vec3 color;
            void main(void)
            {
                float pixelSize = pixelSizes[0];
                vec4 right = vec4(0, 0.5*pixelSize, 0, 0);
                vec4 up = vec4(0.5*pixelSize, 0, 0, 0);
                color = colors[0];
                gl_Position = gl_in[0].gl_Position - projectionMatrix*(right + up);
                EmitVertex();
                gl_Position = gl_in[0].gl_Position - projectionMatrix*(right - up);
                EmitVertex();
                gl_Position = gl_in[0].gl_Position + projectionMatrix*(right - up);
                EmitVertex();
                gl_Position = gl_in[0].gl_Position + projectionMatrix*(right + up);
                EmitVertex();
                EndPrimitive();
            }
        )");

        // build the material
        renderPass->setShaderProgram(shaderProgram);
        technique->addRenderPass(renderPass);
        effect->addTechnique(technique);
        _cloudMaterial->setEffect(effect);
        break;
    }
    case DisplayMode::Triangles:
    {
        // _cloudMaterial = new QMaterial(this);
        _cloudMaterial = new QPerVertexColorMaterial(this);
        // _cloudMaterial = new QPhongMaterial(this);
        /*
        // configure cloud material
        QEffect* effect = new QEffect;
        QTechnique* technique = new QTechnique;
        QRenderPass* renderPass = new QRenderPass;
        QShaderProgram* shaderProgram = new QShaderProgram;

        // set vertex shader
        shaderProgram->setVertexShaderCode(R"(#version 330 core
            uniform mat4 modelViewProjection;
            in vec3 vertexPosition;
            in vec3 vertexColor;
            out vec3 colors;
            in float pixelSize;
            out float pixelSizes;
            void main(void)
            {
                gl_Position = modelViewProjection * vec4(vertexPosition, 1.0f);
                colors = vertexColor;
                pixelSizes = pixelSize;
            }
        )");

        // set fragment shader
        shaderProgram->setFragmentShaderCode(R"(#version 330 core
            in vec3 color;
            out vec4 fragColor;
            void main(void)
            {
                fragColor = vec4(color, 1.0);
            }
        )");*/

        /*
        _cloudMaterial = new QMaterial(this);

        // configure cloud material
        QEffect* effect = new QEffect;
        QTechnique* technique = new QTechnique;
        QRenderPass* renderPass = new QRenderPass;
        QShaderProgram* shaderProgram = new QShaderProgram;

        // set vertex shader
        shaderProgram->setVertexShaderCode(R"(
           #version 330 core

           in vec3 vertexPosition;
           in vec3 vertexNormal;

           out EyeSpaceVertex {
               vec3 position;
               vec3 normal;
           } vs_out;

           uniform mat4 modelView;
           uniform mat3 modelViewNormal;
           uniform mat4 mvp;

           void main()
           {
               vs_out.normal = normalize( modelViewNormal * vertexNormal );
               vs_out.position = vec3( modelView * vec4( vertexPosition, 1.0 ) );

               gl_Position = mvp * vec4( vertexPosition, 1.0 );
           }
        )");

        // set fragment shader
        shaderProgram->setFragmentShaderCode(R"(
            #version 330 core

            uniform struct LightInfo {
             vec4 position;
             vec3 intensity;
            } light;

            uniform struct LineInfo {
             float width;
             vec4 color;
            } line;

            uniform vec3 ka;            // Ambient reflectivity
            uniform vec3 kd;            // Diffuse reflectivity
            uniform vec3 ks;            // Specular reflectivity
            uniform float shininess;    // Specular shininess factor

            in WireframeVertex {
             vec3 position;
             vec3 normal;
             noperspective vec4 edgeA;
             noperspective vec4 edgeB;
             flat int configuration;
            } fs_in;

            out vec4 fragColor;

            vec3 adsModel( const in vec3 pos, const in vec3 n )
            {
             // Calculate the vector from the light to the fragment
             vec3 s = normalize( vec3( light.position ) - pos );

             // Calculate the vector from the fragment to the eye position (the
             // origin since this is in "eye" or "camera" space
             vec3 v = normalize( -pos );

             // Refleft the light beam using the normal at this fragment
             vec3 r = reflect( -s, n );

             // Calculate the diffus component
             vec3 diffuse = vec3( max( dot( s, n ), 0.0 ) );

             // Calculate the specular component
             vec3 specular = vec3( pow( max( dot( r, v ), 0.0 ), shininess ) );

             // Combine the ambient, diffuse and specular contributions
             return light.intensity * ( ka + kd * diffuse + ks * specular );
            }

            vec4 shadeLine( const in vec4 color )
            {
             // Find the smallest distance between the fragment and a triangle edge
             float d;
             if ( fs_in.configuration == 0 )
             {
                 // Common configuration
                 d = min( fs_in.edgeA.x, fs_in.edgeA.y );
                 d = min( d, fs_in.edgeA.z );
             }
             else
             {
                 // Handle configuration where screen space projection breaks down
                 // Compute and compare the squared distances
                 vec2 AF = gl_FragCoord.xy - fs_in.edgeA.xy;
                 float sqAF = dot( AF, AF );
                 float AFcosA = dot( AF, fs_in.edgeA.zw );
                 d = abs( sqAF - AFcosA * AFcosA );

                 vec2 BF = gl_FragCoord.xy - fs_in.edgeB.xy;
                 float sqBF = dot( BF, BF );
                 float BFcosB = dot( BF, fs_in.edgeB.zw );
                 d = min( d, abs( sqBF - BFcosB * BFcosB ) );

                 // Only need to care about the 3rd edge for some configurations.
                 if ( fs_in.configuration == 1 || fs_in.configuration == 2 || fs_in.configuration == 4 )
                 {
                     float AFcosA0 = dot( AF, normalize( fs_in.edgeB.xy - fs_in.edgeA.xy ) );
                     d = min( d, abs( sqAF - AFcosA0 * AFcosA0 ) );
                 }

                 d = sqrt( d );
             }

             // Blend between line color and phong color
             float mixVal;
             if ( d < line.width - 1.0 )
             {
                 mixVal = 1.0;
             }
             else if ( d > line.width + 1.0 )
             {
                 mixVal = 0.0;
             }
             else
             {
                 float x = d - ( line.width - 1.0 );
                 mixVal = exp2( -2.0 * ( x * x ) );
             }

             return mix( color, line.color, mixVal );
            }

            void main()
            {
             // Calculate the color from the phong model
             vec4 color = vec4( adsModel( fs_in.position, normalize( fs_in.normal ) ), 1.0 );
             fragColor = shadeLine( color );
            }
        )");

        // set geometry shader
        shaderProgram->setGeometryShaderCode(R"(
            #version 330 core

            layout( triangles ) in;
            layout( triangle_strip, max_vertices = 3 ) out;

            in EyeSpaceVertex {
             vec3 position;
             vec3 normal;
            } gs_in[];

            out WireframeVertex {
             vec3 position;
             vec3 normal;
             noperspective vec4 edgeA;
             noperspective vec4 edgeB;
             flat int configuration;
            } gs_out;

            uniform mat4 viewportMatrix;

            const int infoA[]  = int[]( 0, 0, 0, 0, 1, 1, 2 );
            const int infoB[]  = int[]( 1, 1, 2, 0, 2, 1, 2 );
            const int infoAd[] = int[]( 2, 2, 1, 1, 0, 0, 0 );
            const int infoBd[] = int[]( 2, 2, 1, 2, 0, 2, 1 );

            vec2 transformToViewport( const in vec4 p )
            {
             return vec2( viewportMatrix * ( p / p.w ) );
            }

            void main()
            {
             gs_out.configuration = int(gl_in[0].gl_Position.z < 0) * int(4)
                    + int(gl_in[1].gl_Position.z < 0) * int(2)
                    + int(gl_in[2].gl_Position.z < 0);

             // If all vertices are behind us, cull the primitive
             if (gs_out.configuration == 7)
                 return;

             // Transform each vertex into viewport space
             vec2 p[3];
             p[0] = transformToViewport( gl_in[0].gl_Position );
             p[1] = transformToViewport( gl_in[1].gl_Position );
             p[2] = transformToViewport( gl_in[2].gl_Position );

             if (gs_out.configuration == 0)
             {
                 // Common configuration where all vertices are within the viewport
                 gs_out.edgeA = vec4(0.0);
                 gs_out.edgeB = vec4(0.0);

                 // Calculate lengths of 3 edges of triangle
                 float a = length( p[1] - p[2] );
                 float b = length( p[2] - p[0] );
                 float c = length( p[1] - p[0] );

                 // Calculate internal angles using the cosine rule
                 float alpha = acos( ( b * b + c * c - a * a ) / ( 2.0 * b * c ) );
                 float beta = acos( ( a * a + c * c - b * b ) / ( 2.0 * a * c ) );

                 // Calculate the perpendicular distance of each vertex from the opposing edge
                 float ha = abs( c * sin( beta ) );
                 float hb = abs( c * sin( alpha ) );
                 float hc = abs( b * sin( alpha ) );

                 // Now add this perpendicular distance as a per-vertex property in addition to
                 // the position and normal calculated in the vertex shader.

                 // Vertex 0 (a)
                 gs_out.edgeA = vec4( ha, 0.0, 0.0, 0.0 );
                 gs_out.normal = gs_in[0].normal;
                 gs_out.position = gs_in[0].position;
                 gl_Position = gl_in[0].gl_Position;
                 EmitVertex();

                 // Vertex 1 (b)
                 gs_out.edgeA = vec4( 0.0, hb, 0.0, 0.0 );
                 gs_out.normal = gs_in[1].normal;
                 gs_out.position = gs_in[1].position;
                 gl_Position = gl_in[1].gl_Position;
                 EmitVertex();

                 // Vertex 2 (c)
                 gs_out.edgeA = vec4( 0.0, 0.0, hc, 0.0 );
                 gs_out.normal = gs_in[2].normal;
                 gs_out.position = gs_in[2].position;
                 gl_Position = gl_in[2].gl_Position;
                 EmitVertex();

                 // Finish the primitive off
                 EndPrimitive();
             }
             else
             {
                 // Viewport projection breaks down for one or two vertices.
                 // Caclulate what we can here and defer rest to fragment shader.
                 // Since this is coherent for the entire primitive the conditional
                 // in the fragment shader is still cheap as all concurrent
                 // fragment shader invocations will take the same code path.

                 // Copy across the viewport-space points for the (up to) two vertices
                 // in the viewport
                 gs_out.edgeA.xy = p[infoA[gs_out.configuration]];
                 gs_out.edgeB.xy = p[infoB[gs_out.configuration]];

                 // Copy across the viewport-space edge vectors for the (up to) two vertices
                 // in the viewport
                 gs_out.edgeA.zw = normalize( gs_out.edgeA.xy - p[ infoAd[gs_out.configuration] ] );
                 gs_out.edgeB.zw = normalize( gs_out.edgeB.xy - p[ infoBd[gs_out.configuration] ] );

                 // Pass through the other vertex attributes
                 gs_out.normal = gs_in[0].normal;
                 gs_out.position = gs_in[0].position;
                 gl_Position = gl_in[0].gl_Position;
                 EmitVertex();

                 gs_out.normal = gs_in[1].normal;
                 gs_out.position = gs_in[1].position;
                 gl_Position = gl_in[1].gl_Position;
                 EmitVertex();

                 gs_out.normal = gs_in[2].normal;
                 gs_out.position = gs_in[2].position;
                 gl_Position = gl_in[2].gl_Position;
                 EmitVertex();

                 // Finish the primitive off
                 EndPrimitive();
             }
            }
        )");

        // build the material
        renderPass->setShaderProgram(shaderProgram);
        technique->addRenderPass(renderPass);
        effect->addTechnique(technique);
        _cloudMaterial->setEffect(effect);
        */
        break;
    }
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
    qDebug() << "[DepthMapEntity] loadDepthMap";
    if(!_source.isValid())
        return;

    qDebug() << "[DepthMapEntity] Load Depth Map: " << _source.toLocalFile();

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
        CArr.doprintf();
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
        iCamArr.doprintf();
    }
    else
    {
        qDebug() << "[DepthMapEntity] missing metadata iCamArr.";
    }

    QString simPath = _source.path().replace("depthMap", "simMap");
    oiio::ImageBuf simBuf;
    if(QUrl(simPath).isValid())
    {
        qDebug() << "[DepthMapEntity] Load Sim Map: " << simPath;
        simBuf.reset(simPath.toStdString(), 0, 0, NULL, &configSpec);
    }
    const oiio::ImageSpec& simSpec = simBuf.spec();
    const bool validSimMap = (simSpec.width == inSpec.width) && (simSpec.height == inSpec.height);

    oiio::ImageBufAlgo::PixelStats stats;
    oiio::ImageBufAlgo::computePixelStats(stats, inBuf);

    std::vector<int> indexPerPixel(inSpec.width * inSpec.height, -1);
    std::vector<Vec3f> positions;
    std::vector<Color32f> colors;
    std::vector<float> pixelSizes;

    for(int y = 0; y < inSpec.height; ++y)
    {
        for(int x = 0; x < inSpec.width; ++x)
        {
            float depthValue = 0.0f;
            inBuf.getpixel(x, y, &depthValue, 1);
            if(depthValue == -1.f)
                continue;

            point3d p = CArr + (iCamArr * point2d((double)x, (double)y)).normalize() * depthValue;

            point3d vect = (iCamArr * point2d(x+1, y)).normalize();
            const double pixelSize = pointLineDistance3D(p, CArr, vect);

            Vec3f position(p.x, p.y, p.z);

            indexPerPixel[y * inSpec.width + x] = positions.size();
            positions.push_back(position);
            pixelSizes.push_back(pixelSize);
            if(validSimMap)
            {
                float simValue = 0.0f;
                simBuf.getpixel(x, y, &simValue, 1);
                Color32f color = getColor32fFromJetColorMapClamp(simValue);
                colors.push_back(color);
            }
            else
            {
                float normalizedDepthValue = (depthValue - stats.min[0]) / (stats.max[0] - stats.min[0]);
                Color32f color = getColor32fFromJetColorMapClamp(normalizedDepthValue);
                colors.push_back(color);
            }
        }
    }

    qDebug() << "[DepthMapEntity] Valid Depth Values: " << positions.size();

    // create a new geometry renderer
    QGeometryRenderer* customMeshRenderer = new QGeometryRenderer;
    QGeometry* customGeometry = new QGeometry;

    // vertices buffer
    std::vector<int> trianglesIndexes;
    if(_displayMode == DisplayMode::Triangles)
    {
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
    }
#ifndef QTOIIO_DEPTHMAP_USE_INDEXES
    if(_displayMode == DisplayMode::Triangles)
    {
        std::vector<Vec3f> triangles;
        triangles.resize(trianglesIndexes.size());
        for(int i = 0; i < trianglesIndexes.size(); ++i)
        {
            triangles[i] = positions[trianglesIndexes[i]];
        }

        QBuffer* trianglesBuffer = new QBuffer(QBuffer::VertexBuffer);
        QByteArray trianglesData((const char*)&triangles[0], triangles.size() * sizeof(Vec3f));
        trianglesBuffer->setData(trianglesData);

        QAttribute* positionAttribute = new QAttribute;
        positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        positionAttribute->setAttributeType(QAttribute::VertexAttribute);
        positionAttribute->setBuffer(trianglesBuffer);
        positionAttribute->setDataType(QAttribute::Float);
        positionAttribute->setDataSize(3);
        positionAttribute->setByteOffset(0);
        positionAttribute->setByteStride(sizeof(Vec3f));
        positionAttribute->setCount(triangles.size());

        customGeometry->addAttribute(positionAttribute);
        // customGeometry->setBoundingVolumePositionAttribute(positionAttribute);
    }
    else
#endif
    {
        QByteArray positionData((const char*)positions[0].m, positions.size() * 3 * sizeof(float));
        QBuffer* vertexDataBuffer = new QBuffer(QBuffer::VertexBuffer);
        vertexDataBuffer->setData(positionData);
        QAttribute* positionAttribute = new QAttribute;
        positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        positionAttribute->setAttributeType(QAttribute::VertexAttribute);
        positionAttribute->setBuffer(vertexDataBuffer);
        positionAttribute->setDataType(QAttribute::Float);
        positionAttribute->setDataSize(3);
        positionAttribute->setByteOffset(0);
        positionAttribute->setByteStride(3 * sizeof(float));
        positionAttribute->setCount(positions.size());

        customGeometry->addAttribute(positionAttribute);
        // customGeometry->setBoundingVolumePositionAttribute(positionAttribute);
    }

    // colors buffer
#ifdef QTOIIO_DEPTHMAP_USE_INDEXES
    {
        // read color data
        QBuffer* colorDataBuffer = new QBuffer(QBuffer::VertexBuffer);
        QByteArray colorData((const char*)colors[0].m, colors.size() * 3 * sizeof(float));
        colorDataBuffer->setData(colorData);

        QAttribute* colorAttribute = new QAttribute;
        qDebug() << "Qt3DRender::QAttribute::defaultColorAttributeName(): " << Qt3DRender::QAttribute::defaultColorAttributeName().toStdString();
        colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
        colorAttribute->setAttributeType(QAttribute::VertexAttribute);
        colorAttribute->setBuffer(colorDataBuffer);
        colorAttribute->setDataType(QAttribute::Float);
        colorAttribute->setDataSize(3);
        colorAttribute->setByteOffset(0);
        colorAttribute->setByteStride(3 * sizeof(float));
        colorAttribute->setCount(positions.size());
        customGeometry->addAttribute(colorAttribute);
    }
#else
    {
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
    }
#endif

    // Pixel Sizes buffer
#ifdef QTOIIO_DEPTHMAP_USE_INDEXES
    {
        QBuffer* pixelSizeBuffer = new QBuffer(QBuffer::VertexBuffer);
        QByteArray pixelSizeData((const char*)&pixelSizes[0], pixelSizes.size() * sizeof(float));
        pixelSizeBuffer->setData(pixelSizeData);

        QAttribute* pixelSizeAttribute = new QAttribute;
        pixelSizeAttribute->setName("pixelSize");
        pixelSizeAttribute->setAttributeType(QAttribute::VertexAttribute);
        pixelSizeAttribute->setBuffer(pixelSizeBuffer);
        pixelSizeAttribute->setDataType(QAttribute::Float);
        pixelSizeAttribute->setDataSize(1);
        pixelSizeAttribute->setByteOffset(0);
        pixelSizeAttribute->setByteStride(sizeof(float));
        pixelSizeAttribute->setCount(pixelSizes.size());
        customGeometry->addAttribute(pixelSizeAttribute);
    }
#else
    {
        // Duplicate pixelSize as we cannot use indexes!
        std::vector<float> pixelSizesFlat;
        pixelSizesFlat.reserve(trianglesIndexes.size());
        for(int i = 0; i < trianglesIndexes.size(); ++i)
        {
            pixelSizesFlat.push_back(pixelSizes[trianglesIndexes[i]]);
        }

        QBuffer* pixelSizeBuffer = new QBuffer(QBuffer::VertexBuffer);
        QByteArray pixelSizeData((const char*)&pixelSizes[0], pixelSizes.size() * sizeof(float));
        pixelSizeBuffer->setData(pixelSizeData);

        QAttribute* pixelSizeAttribute = new QAttribute;
        pixelSizeAttribute->setName("pixelSize");
        pixelSizeAttribute->setAttributeType(QAttribute::VertexAttribute);
        pixelSizeAttribute->setBuffer(pixelSizeBuffer);
        pixelSizeAttribute->setDataType(QAttribute::Float);
        pixelSizeAttribute->setDataSize(1);
        pixelSizeAttribute->setByteOffset(0);
        pixelSizeAttribute->setByteStride(sizeof(float));
        pixelSizeAttribute->setCount(pixelSizes.size());
        customGeometry->addAttribute(pixelSizeAttribute);
    }
#endif

    // Triangles
#ifdef QTOIIO_DEPTHMAP_USE_INDEXES
    if(_displayMode == DisplayMode::Triangles)
    {
        QBuffer* trianglesBuffer = new QBuffer(QBuffer::IndexBuffer);
        QByteArray trianglesData((const char*)&trianglesIndexes[0], trianglesIndexes.size() * sizeof(int));
        trianglesBuffer->setData(trianglesData);

        QAttribute *indexAttribute = new QAttribute();
        // indexAttribute->setName(Qt3DRender::QAttribute::defaultJointIndicesAttributeName());
        indexAttribute->setAttributeType(QAttribute::IndexAttribute);
        indexAttribute->setBuffer(trianglesBuffer);
        indexAttribute->setDataType(QAttribute::Int);
        // indexAttribute->setDataSize(1);
        // indexAttribute->setByteOffset(0);
        // indexAttribute->setByteStride(sizeof(int));
        indexAttribute->setCount(trianglesIndexes.size());

        customGeometry->addAttribute(indexAttribute);
        //customMeshRenderer->setVertexCount(triangles.size());
    }
#endif

    // geometry renderer settings
    switch(_displayMode)
    {
    case DisplayMode::Points:
        customMeshRenderer->setPrimitiveType(QGeometryRenderer::Points);
        break;
    case DisplayMode::Triangles:
        customMeshRenderer->setPrimitiveType(QGeometryRenderer::Triangles);
        break;
    }
    // customMeshRenderer->setInstanceCount(1);
    // customMeshRenderer->setIndexOffset(0);
    // customMeshRenderer->setFirstInstance(0);
    // customMeshRenderer->setFirstVertex(0);
    // customMeshRenderer->setIndexBufferByteOffset(0);
    customMeshRenderer->setGeometry(customGeometry);

    // add components
    Qt3DCore::QTransform* transform = new Qt3DCore::QTransform;
    // QMatrix4x4 qmat;
    // transform->setMatrix(qmat);

    addComponent(_cloudMaterial);
    addComponent(transform);
    addComponent(customMeshRenderer);
    qDebug() << "DepthMapEntity: Mesh Renderer added.";
}

} // namespace
