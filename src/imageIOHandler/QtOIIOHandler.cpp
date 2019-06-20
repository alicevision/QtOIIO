#include "QtOIIOHandler.hpp"

#include "../jetColorMap.hpp"

#include <QImage>
#include <QIODevice>
#include <QFileDevice>
#include <QVariant>
#include <QDataStream>
#include <QDebug>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <iostream>
#include <memory>

namespace oiio = OIIO;

QtOIIOHandler::QtOIIOHandler()
{
    qDebug() << "[QtOIIO] QtOIIOHandler";
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
        // qDebug() << "[QtOIIO] Can read file: " << d->fileName().toStdString();
        return true;
    }
    // qDebug() << "[QtOIIO] Cannot read.";
    return false;
}

bool QtOIIOHandler::read(QImage *image)
{
    bool convertGrayscaleToJetColorMap = true; // how to expose it as an option?

    // qDebug() << "[QtOIIO] Read Image";
    QFileDevice* d = dynamic_cast<QFileDevice*>(device());
    if(!d)
    {
        qWarning() << "[QtOIIO] Read image failed (not a FileDevice).";
        return false;
    }
    const std::string path = d->fileName().toStdString();

    qInfo() << "[QtOIIO] Read image: " << path.c_str();
    // check requested channels number
    // assert(nchannels == 1 || nchannels >= 3);

    oiio::ImageSpec configSpec;
    // libRAW configuration
    configSpec.attribute("raw:auto_bright", 0);       // don't want exposure correction
    configSpec.attribute("raw:use_camera_wb", 1);     // want white balance correction
#if OIIO_VERSION <= (10000 * 2 + 100 * 0 + 8) // OIIO_VERSION <= 2.0.8
    // In these previous versions of oiio, there was no Linear option
    configSpec.attribute("raw:ColorSpace", "sRGB");   // want colorspace sRGB
#else
    configSpec.attribute("raw:ColorSpace", "Linear");   // want linear colorspace with sRGB primaries
#endif    
    configSpec.attribute("raw:use_camera_matrix", 3); // want to use embeded color profile

    oiio::ImageBuf inBuf(path, 0, 0, NULL, &configSpec);

    if(!inBuf.initialized())
        throw std::runtime_error("Can't find/open image file '" + path + "'.");

#if OIIO_VERSION <= (10000 * 2 + 100 * 0 + 8) // OIIO_VERSION <= 2.0.8
    // Workaround for bug in RAW colorspace management in previous versions of OIIO:
    //     When asking sRGB we got sRGB primaries with linear gamma,
    //     but oiio::ColorSpace was wrongly set to sRGB.
    oiio::ImageSpec inSpec = inBuf.spec();
    if(inSpec.get_string_attribute("oiio:ColorSpace", "") == "sRGB")
    {
        const auto in = oiio::ImageInput::open(path, nullptr);
        const std::string formatStr = in->format_name();
        if(formatStr == "raw")
        {
            // For the RAW plugin: override colorspace as linear (as the content is linear with sRGB primaries but declared as sRGB)
            inSpec.attribute("oiio:ColorSpace", "Linear");
            qDebug() << "OIIO workaround: RAW input image " << QString::fromStdString(path) << " is in Linear.";
        }
    }
#else
    const oiio::ImageSpec& inSpec = inBuf.spec();
#endif

    qDebug() << "[QtOIIO] width:" << inSpec.width << ", height:" << inSpec.height << ", nchannels:" << inSpec.nchannels;

    if(inSpec.nchannels >= 3)
    {
        // Color conversion to sRGB
        const std::string& colorSpace = inSpec.get_string_attribute("oiio:ColorSpace", "sRGB"); // default image color space is sRGB

        if(colorSpace != "sRGB") // color conversion to sRGB
        {
            oiio::ImageBufAlgo::colorconvert(inBuf, inBuf, colorSpace, "sRGB");
            qDebug() << "Convert image " << QString::fromStdString(path) << " from " << QString::fromStdString(colorSpace) << " to sRGB colorspace";
        }
    }

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
        qWarning() << "[QtOIIO] failed to load \"" << path.c_str() << "\", nchannels=" << inSpec.nchannels;
        return false;
    }

    qDebug() << "[QtOIIO] nchannels:" << nchannels;

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
            qDebug() << "[QtOIIO] compute colormap \"" << colorMapType.c_str() << "\"";
            oiio::ImageBufAlgo::color_map(tmpBuf, inBuf, 0, colorMapType);
        }
        else if(d->fileName().contains("depthMap"))
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
        else if(d->fileName().contains("nmodMap"))
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
        // qDebug() << "[QtOIIO] compute colormap done";
        inBuf.swap(tmpBuf);
    }

    // Shuffle channels to convert from OIIO to Qt
    else if(nchannels == 4)
    {
        // qDebug() << "[QtOIIO] shuffle channels";
        oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
        oiio::ImageBuf tmpBuf(requestedSpec);

        const std::vector<int> channelOrder = {2, 1, 0, 3}; // This one works, not sure why...
        oiio::ImageBufAlgo::channels(tmpBuf, inBuf, 4, &channelOrder.front());
        inBuf.swap(tmpBuf);
        // qDebug() << "[QtOIIO] shuffle channels done";
    }

    // qDebug() << "[QtOIIO] create output QImage";
    QImage result(inSpec.width, inSpec.height, format);

    {
        oiio::ROI exportROI = inBuf.roi();
        exportROI.chbegin = 0;
        exportROI.chend = nchannels;

        // qDebug() << "[QtOIIO] fill output QImage";
        inBuf.get_pixels(exportROI, typeDesc, result.bits());
    }

    // qDebug() << "[QtOIIO] Image loaded: \"" << path << "\"";
    if (_scaledSize.isValid())
    {
        qDebug() << "[QTOIIO] _scaledSize: " << _scaledSize.width() << "x" << _scaledSize.height();
        *image = result.scaled(_scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        *image = result;
    }
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
    if(option == ImageTransformation)
        return true;
    if(option == ScaledSize)
        return true;

    return false;
}

QVariant QtOIIOHandler::option(ImageOption option) const
{
    const auto getImageInput = [](QIODevice* device) -> std::unique_ptr<oiio::ImageInput> {
        QFileDevice* d = dynamic_cast<QFileDevice*>(device);
        if(!d)
        {
            qDebug() << "[QtOIIO] Read image failed (not a FileDevice).";
            return std::unique_ptr<oiio::ImageInput>(nullptr);
        }
        std::string path = d->fileName().toStdString();
        return std::unique_ptr<oiio::ImageInput>(oiio::ImageInput::open(path));
    };

    if (option == Size)
    {
        std::unique_ptr<oiio::ImageInput> imageInput = getImageInput(device());
        if(imageInput.get() == nullptr)
            return QVariant();

        return QSize(imageInput->spec().width, imageInput->spec().height);
    }
    else if(option == ImageTransformation)
    {
        std::unique_ptr<oiio::ImageInput> imageInput = getImageInput(device());
        if(imageInput.get() == nullptr)
        {
            return QImageIOHandler::TransformationNone;
        }
        // Translate OIIO transformations to QImageIOHandler::ImageTransformation
        switch(oiio::ImageBuf(imageInput->spec()).orientation())
        {
        case 1: return QImageIOHandler::TransformationNone; break;
        case 2: return QImageIOHandler::TransformationMirror; break;
        case 3: return QImageIOHandler::TransformationRotate180; break;
        case 4: return QImageIOHandler::TransformationFlip; break;
        case 5: return QImageIOHandler::TransformationMirrorAndRotate90; break;
        case 6: return QImageIOHandler::TransformationRotate90; break;
        case 7: return QImageIOHandler::TransformationFlipAndRotate90; break;
        case 8: return QImageIOHandler::TransformationRotate270; break;
        }
    }
    return QImageIOHandler::option(option);
}

void QtOIIOHandler::setOption(ImageOption option, const QVariant &value)
{
    Q_UNUSED(option);
    Q_UNUSED(value);
    if (option == ScaledSize && value.isValid())
    {
        _scaledSize = value.value<QSize>();
        qDebug() << "[QTOIIO] setOption scaledSize: " << _scaledSize.width() << "x" << _scaledSize.height();
    }
}

QByteArray QtOIIOHandler::name() const
{
    return "OpenImageIO";
}
