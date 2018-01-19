#include "QtOIIOHandler.hpp"

#include <QImage>
#include <QIODevice>
#include <QFileDevice>
#include <QVariant>
#include <QDataStream>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <iostream>

namespace oiio = OIIO;



static float jetr[64] = {0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0.0625, 0.1250, 0.1875, 0.2500, 0.3125, 0.3750, 0.4375, 0.5000, 0.5625,
                         0.6250, 0.6875, 0.7500, 0.8125, 0.8750, 0.9375, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 0.9375, 0.8750, 0.8125, 0.7500, 0.6875, 0.6250, 0.5625, 0.5000};

static float jetg[64] = {0,      0,      0,      0,      0,      0,      0,      0,      0.0625, 0.1250, 0.1875,
                         0.2500, 0.3125, 0.3750, 0.4375, 0.5000, 0.5625, 0.6250, 0.6875, 0.7500, 0.8125, 0.8750,
                         0.9375, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 0.9375, 0.8750, 0.8125, 0.7500,
                         0.6875, 0.6250, 0.5625, 0.5000, 0.4375, 0.3750, 0.3125, 0.2500, 0.1875, 0.1250, 0.0625,
                         0,      0,      0,      0,      0,      0,      0,      0,      0};

static float jetb[64] = {0.5625, 0.6250, 0.6875, 0.7500, 0.8125, 0.8750, 0.9375, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 0.9375, 0.8750, 0.8125, 0.7500, 0.6875, 0.6250, 0.5625, 0.5000, 0.4375,
                         0.3750, 0.3125, 0.2500, 0.1875, 0.1250, 0.0625, 0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0};

struct Color32f
{
    Color32f() {}
    Color32f(float r_, float g_, float b_)
      : r(r_)
      , g(g_)
      , b(b_)
    {}
    union {
        struct
        {
            float r, g, b;
        };
        float m[3];
    };
};

Color32f getColor32fFromJetColorMap(float value)
{
    if(value <= 0.0f)
        return Color32f(0, 0, 0);
    if(value >= 1.0f)
        return Color32f(1.0f, 1.0f, 1.0f);
    float idx_f = value * 63.0f;
    float fractA, fractB, integral;
    fractB = std::modf(idx_f, &integral);
    fractA = 1.0f - fractB;
    int idx = static_cast<int>(integral);
    Color32f c;
    c.r = jetr[idx] * fractA + jetr[idx + 1] * fractB;
    c.g = jetg[idx] * fractA + jetg[idx + 1] * fractB;
    c.b = jetb[idx] * fractA + jetb[idx + 1] * fractB;
    return c;
}

Color32f getColor32fFromJetColorMapClamp(float value)
{
    if(value < 0.0f)
        value = 0.0f;
    if(value > 1.0f)
        value = 1.0f;
    float idx_f = value * 63.0f;
    float fractA, fractB, integral;
    fractB = std::modf(idx_f, &integral);
    fractA = 1.0f - fractB;
    int idx = static_cast<int>(integral);
    Color32f c;
    c.r = jetr[idx] * fractA + jetr[idx + 1] * fractB;
    c.g = jetg[idx] * fractA + jetg[idx + 1] * fractB;
    c.b = jetb[idx] * fractA + jetb[idx + 1] * fractB;
    return c;
}


QtOIIOHandler::QtOIIOHandler()
{
    std::cout << "[QtOIIO] QtOIIOHandler" << std::endl;
}

QtOIIOHandler::~QtOIIOHandler()
{
}

bool QtOIIOHandler::canRead() const
{
    if(canRead(device()))
    {
        setFormat("OpenImageIO");
        return true;
    }
    return false;
}

bool QtOIIOHandler::canRead(QIODevice *device)
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(d)
    {
        // std::cout << "[QtOIIO] Can read file: " << d->fileName().toStdString() << std::endl;
        return true;
    }
    // std::cout << "[QtOIIO] Cannot read." << std::endl;
    return false;
}

bool QtOIIOHandler::read(QImage *image)
{
    bool convertGrayscaleToJetColorMap = true; // how to expose it as an option?

    // std::cout << "[QtOIIO] Read Image" << std::endl;
    QFileDevice* d = dynamic_cast<QFileDevice*>(device());
    if(!d)
    {
        std::cout << "[QtOIIO] Read image failed (not a FileDevice)." << std::endl;
        return false;
    }
    const std::string path = d->fileName().toStdString();

    std::cout << "[QtOIIO] Read image: " << path << std::endl;
    // check requested channels number
    // assert(nchannels == 1 || nchannels >= 3);

    oiio::ImageSpec configSpec;
    // libRAW configuration
    configSpec.attribute("raw:auto_bright", 0);       // don't want exposure correction
    configSpec.attribute("raw:use_camera_wb", 1);     // want white balance correction
    configSpec.attribute("raw:ColorSpace", "sRGB");   // want colorspace sRGB
    configSpec.attribute("raw:use_camera_matrix", 3); // want to use embeded color profile

    oiio::ImageBuf inBuf(path, 0, 0, NULL, &configSpec);

    if(!inBuf.initialized())
        throw std::runtime_error("Can't find/open image file '" + path + "'.");

    const oiio::ImageSpec& inSpec = inBuf.spec();

    std::cout << "[QtOIIO] width:" << inSpec.width << ", height:" << inSpec.height << ", nchannels:" << inSpec.nchannels << std::endl;

    int nchannels = 0;
    QImage::Format format = QImage::NImageFormats;
    if(inSpec.nchannels == 4)
    {
        format = QImage::Format_ARGB32; // Qt documentation: The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        nchannels = 4;
    }
    else if(inSpec.nchannels == 3)
    {
        format = QImage::Format_RGB32; // Qt documentation: The image is stored using a 32-bit RGB format (0xffRRGGBB).
        nchannels = 4;
    }
    else if(inSpec.nchannels == 1)
    {
        if(convertGrayscaleToJetColorMap)
        {
            format = QImage::Format_RGB32; // Qt documentation: The image is stored using a 32-bit RGB format (0xffRRGGBB).
            nchannels = 4;
        }
        else
        {
            format = QImage::Format_Grayscale8; // Qt documentation: The image is stored using an 8-bit grayscale format.
            nchannels = 1;
        }
    }
    else
    {
        std::cout << "[QtOIIO] failed to load \"" << path << "\", nchannels=" << inSpec.nchannels << std::endl;
        return false;
    }

    std::cout << "[QtOIIO] nchannels:" << nchannels << std::endl;

    // check picture channels number
    if(inSpec.nchannels < 3 && inSpec.nchannels != 1)
        throw std::runtime_error("Can't load channels of image file '" + path + "'.");

//    // convert to grayscale if needed
//    if(nchannels == 1 && inSpec.nchannels >= 3)
//    {
//        // convertion region of interest (for inSpec.nchannels > 3)
//        oiio::ROI convertionROI = inBuf.roi();
//        convertionROI.chbegin = 0;
//        convertionROI.chend = 3;

//        // compute luminance via a weighted sum of R,G,B
//        // (assuming Rec709 primaries and a linear scale)
//        const float weights[3] = {.2126, .7152, .0722};
//        oiio::ImageBuf grayscaleBuf;
//        oiio::ImageBufAlgo::channel_sum(grayscaleBuf, inBuf, weights, convertionROI);
//        inBuf.copy(grayscaleBuf);
//    }

    oiio::TypeDesc typeDesc = oiio::TypeDesc::UINT8;
//    // add missing channels
//    if(inSpec.nchannels < 3 && nchannels > inSpec.nchannels)
//    {
//        oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
//        oiio::ImageBuf requestedBuf(requestedSpec);

//        // duplicate first channel for RGB
//        if(requestedSpec.nchannels >= 3 && inSpec.nchannels < 3)
//        {
//            oiio::ImageBufAlgo::paste(requestedBuf, 0, 0, 0, 0, inBuf);
//            oiio::ImageBufAlgo::paste(requestedBuf, 0, 0, 0, 1, inBuf);
//            oiio::ImageBufAlgo::paste(requestedBuf, 0, 0, 0, 2, inBuf);
//        }
//        inBuf.swap(requestedBuf);
//    }
    // if the input is grayscale, we have the option to convert it with a color map
    if(convertGrayscaleToJetColorMap && inSpec.nchannels == 1)
    {
        oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
        oiio::ImageBuf tmpBuf(requestedSpec);
        // perceptually uniform: "inferno", "viridis", "magma", "plasma" -- others: "blue-red", "spectrum", "heat"
        const char* colorMapEnv = std::getenv("QTOIIO_COLORMAP");
        const std::string colorMapType = colorMapEnv ? colorMapEnv : "plasma";
        if(colorMapEnv)
        {
            std::cout << "[QtOIIO] compute colormap \"" << colorMapType << "\"" << std::endl;
            oiio::ImageBufAlgo::color_map(tmpBuf, inBuf, 0, colorMapType);
        }
        else if(d->fileName().endsWith("_depthMap.exr"))
        {
            oiio::ImageBufAlgo::PixelStats stats;
            oiio::ImageBufAlgo::computePixelStats(stats, inBuf);

#pragma omp parallel for
            for(int y = 0; y < inSpec.height; ++y)
            {
                for(int x = 0; x < inSpec.width; ++x)
                {
                    float depthValue = 0.0f;
                    inBuf.getpixel(x, y, &depthValue, 1);
                    float normalizedDepthValue = (depthValue - stats.min[0]) / (stats.max[0] - stats.min[0]);
                    Color32f color = getColor32fFromJetColorMap(normalizedDepthValue);
                    tmpBuf.setpixel(x, y, color.m, 3); // set only 3 channels (RGB)
                }
            }
        }
        else if(d->fileName().endsWith("_nmodMap.png"))
        {
            oiio::ImageBufAlgo::PixelStats stats;
            oiio::ImageBufAlgo::computePixelStats(stats, inBuf);
            // oiio::ImageBufAlgo::color_map(dst, src, srcchannel, int(knots.size()/3), 3, knots);

#pragma omp parallel for
            for(int y = 0; y < inSpec.height; ++y)
            {
                for(int x = 0; x < inSpec.width; ++x)
                {
                    float depthValue = 0.0f;
                    inBuf.getpixel(x, y, &depthValue, 1);
                    float normalizedDepthValue = (depthValue - stats.min[0]) / (stats.max[0] - stats.min[0]);
                    Color32f color = getColor32fFromJetColorMapClamp(normalizedDepthValue);
                    tmpBuf.setpixel(x, y, color.m, 3); // set only 3 channels (RGB)
                }
            }
        }
        else
        {
#pragma omp parallel for
            for(int y = 0; y < inSpec.height; ++y)
            {
                for(int x = 0; x < inSpec.width; ++x)
                {
                    float depthValue = 0.0f;
                    inBuf.getpixel(x, y, &depthValue, 1);
                    Color32f color = getColor32fFromJetColorMap(depthValue);
                    tmpBuf.setpixel(x, y, color.m, 3); // set only 3 channels (RGB)
                }
            }
        }
        // std::cout << "[QtOIIO] compute colormap done" << std::endl;
        inBuf.swap(tmpBuf);
    }

    // Shuffle channels to convert from OIIO to Qt
    else if(nchannels == 4)
    {
        // std::cout << "[QtOIIO] shuffle channels" << std::endl;
        oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
        oiio::ImageBuf tmpBuf(requestedSpec);

        const std::vector<int> channelOrder = {2, 1, 0, 3}; // This one works, not sure why...
        oiio::ImageBufAlgo::channels(tmpBuf, inBuf, 4, &channelOrder.front());
        inBuf.swap(tmpBuf);
        // std::cout << "[QtOIIO] shuffle channels done" << std::endl;
    }

    // std::cout << "[QtOIIO] create output QImage" << std::endl;
    QImage result(inSpec.width, inSpec.height, format);

    {
        oiio::ROI exportROI = inBuf.roi();
        exportROI.chbegin = 0;
        exportROI.chend = nchannels;

        // std::cout << "[QtOIIO] fill output QImage" << std::endl;
        inBuf.get_pixels(exportROI, typeDesc, result.bits());
    }

    // std::cout << "[QtOIIO] Image loaded: \"" << path << "\"" << std::endl;
    *image = result;
    return true;
}

bool QtOIIOHandler::write(const QImage &image)
{
    // TODO
    return false;
}

bool QtOIIOHandler::supportsOption(ImageOption option) const
{
    if(option == Size)
        return true;
    if(option == TransformedByDefault)
        return true;
    return false;
}

QVariant QtOIIOHandler::option(ImageOption option) const
{
    if (option == Size)
    {
        QFileDevice* d = dynamic_cast<QFileDevice*>(device());
        if(!d)
        {
            std::cout << "[QtOIIO] Read image failed (not a FileDevice)." << std::endl;
            return false;
        }
        std::string path = d->fileName().toStdString();

        std::unique_ptr<oiio::ImageInput> imageInput(oiio::ImageInput::open(path));
        if(imageInput.get() == nullptr)
            return QVariant();

        return QSize(imageInput->spec().width, imageInput->spec().height);
    }
    return QVariant();
}

void QtOIIOHandler::setOption(ImageOption option, const QVariant &value)
{
    Q_UNUSED(option);
    Q_UNUSED(value);
}

QByteArray QtOIIOHandler::name() const
{
    return "OpenImageIO";
}
