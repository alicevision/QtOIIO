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

inline const float& clamp( const float& v, const float& lo, const float& hi )
{
    assert( !(hi < lo) );
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

inline unsigned short floatToUShort(float v)
{
    return clamp(v, 0.0f, 1.0f) * 65535;
}

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
    configSpec.attribute("raw:user_flip", 1);
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
    float pixelAspectRatio = inSpec.get_float_attribute("PixelAspectRatio", 1.0f);

    qInfo() << "[QtOIIO] width:" << inSpec.width << ", height:" << inSpec.height << ", nchannels:" << inSpec.nchannels << ", format:" << inSpec.format.c_str();

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
    const bool moreThan8Bits = inSpec.format != oiio::TypeDesc::UINT8 && inSpec.format != oiio::TypeDesc::INT8;
    QImage::Format format = QImage::NImageFormats;
    if(inSpec.nchannels == 4)
    {
        if(moreThan8Bits)
            format = QImage::Format_RGBA64; // Qt documentation: The image is stored using a 64-bit halfword-ordered RGBA format (16-16-16-16). (added in Qt 5.12)
        else
            format = QImage::Format_ARGB32; // Qt documentation: The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        nchannels = 4;
    }
    else if(inSpec.nchannels == 3)
    {
        if(moreThan8Bits)
            format = QImage::Format_RGBX64; // Qt documentation: The image is stored using a 64-bit halfword-ordered RGB(x) format (16-16-16-16). This is the same as the Format_RGBA64 except alpha must always be 65535. (added in Qt 5.12)
        else
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
            if(moreThan8Bits)
                format = QImage::Format_Grayscale16; // Qt documentation: The image is stored using an 16-bit grayscale format. (added in Qt 5.13)
            else
                format = QImage::Format_Grayscale8; // Qt documentation: The image is stored using an 8-bit grayscale format.
            nchannels = 1;
        }
    }
    else
    {
        qWarning() << "[QtOIIO] failed to load \"" << path.c_str() << "\", nchannels=" << inSpec.nchannels;
        return false;
    }

    {
        std::string formatStr;
        switch(format)
        {
            case QImage::Format_RGBA64: formatStr = "Format_RGBA64"; break;
            case QImage::Format_ARGB32: formatStr = "Format_ARGB32"; break;
            case QImage::Format_RGBX64: formatStr = "Format_RGBX64"; break;
            case QImage::Format_RGB32: formatStr = "Format_RGB32"; break;
            case QImage::Format_Grayscale16: formatStr = "Format_Grayscale16"; break;
            case QImage::Format_Grayscale8: formatStr = "Format_Grayscale8"; break;
            default:
                formatStr = std::string("Unknown QImage Format:") + std::to_string(int(format));
        }
        qDebug() << "[QtOIIO] QImage Format: " << formatStr.c_str();
    }

    qDebug() << "[QtOIIO] nchannels:" << nchannels;

    // check picture channels number
    if(inSpec.nchannels < 3 && inSpec.nchannels != 1)
    {
        qWarning() << "[QtOIIO] Cannot load channels of image file '" << path.c_str() << "' (nchannels=" << inSpec.nchannels << ").";
        return false;
    }

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

    // qDebug() << "[QtOIIO] create output QImage";
    QImage result(inSpec.width, inSpec.height, format);

    // if the input is grayscale, we have the option to convert it with a color map
    if(convertGrayscaleToJetColorMap && inSpec.nchannels == 1)
    {
        const oiio::TypeDesc typeDesc = oiio::TypeDesc::UINT8;
        oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
        oiio::ImageBuf tmpBuf(requestedSpec);
        // perceptually uniform: "inferno", "viridis", "magma", "plasma" -- others: "blue-red", "spectrum", "heat"
        const char* colorMapEnv = std::getenv("QTOIIO_COLORMAP");
        const std::string colorMapType = colorMapEnv ? colorMapEnv : "plasma";

        // detect AliceVision special files that require a jetColorMap based conversion
        const bool isDepthMap = d->fileName().contains("depthMap");
        const bool isNmodMap = d->fileName().contains("nmodMap");

        if(colorMapEnv)
        {
            qDebug() << "[QtOIIO] compute colormap \"" << colorMapType.c_str() << "\"";
            oiio::ImageBufAlgo::color_map(tmpBuf, inBuf, 0, colorMapType);
        }
        else if(isDepthMap || isNmodMap)
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
                    const float range = stats.max[0] - stats.min[0];
                    const float normalizedDepthValue = range != 0.0f ? (depthValue - stats.min[0]) / range : 1.0f;
                    Color32f color;
                    if(isDepthMap)
                        color = getColor32fFromJetColorMap(normalizedDepthValue);
                    else if(isNmodMap)
                        color = getColor32fFromJetColorMapClamp(normalizedDepthValue);
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

        {
            oiio::ROI exportROI = inBuf.roi();
            exportROI.chbegin = 0;
            exportROI.chend = nchannels;

            // qDebug() << "[QtOIIO] fill output QImage";
            inBuf.get_pixels(exportROI, typeDesc, result.bits());
        }
    }

    // Shuffle channels to convert from OIIO to Qt
    else if(nchannels == 4)
    {
        const oiio::TypeDesc typeDesc = moreThan8Bits ? oiio::TypeDesc::UINT16 : oiio::TypeDesc::UINT8;

        if(moreThan8Bits) // same than: format == QImage::Format_RGBA64 || format == QImage::Format_RGBX64
        {
            qDebug() << "[QtOIIO] Convert '" << inSpec.format.c_str() << "'' OIIO image to 'uint16' Qt image.";
#pragma omp parallel for
            for(int y = 0; y < inSpec.height; ++y)
            {
                for(int x = 0; x < inSpec.width; ++x)
                {
                    float rgba[4] = {0.0, 0.0, 0.0, 1.0};
                    inBuf.getpixel(x, y, rgba, 4);

                    quint64* p = (quint64*)(result.scanLine(y)) + x;
                    QRgba64 color = QRgba64::fromRgba64(floatToUShort(rgba[0]), floatToUShort(rgba[1]), floatToUShort(rgba[2]), floatToUShort(rgba[3]));
                    *p = (quint64)color;
                }
            }
        }
        else
        {
            oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
            oiio::ImageBuf tmpBuf(requestedSpec);
            // qDebug() << "[QtOIIO] shuffle channels";

            std::vector<int> channelOrder = {0, 1, 2, 3};
            float channelValues[] = {1.f, 1.f, 1.f, 1.f};
            if(format == QImage::Format_ARGB32)
            {
                // (0xAARRGGBB) => 3, 0, 1, 2 in reverse order
                channelOrder = {2, 1, 0, 3};
            }
            else if(format == QImage::Format_RGB32)
            {
                channelOrder = {2, 1, 0, -1}; // not sure
            }
            else
            {
                qWarning() << "Unsupported format conversion.";
            }
            // qWarning() << "channelOrder: " << channelOrder[0] << ", " << channelOrder[1] << ", " << channelOrder[2] << ", " << channelOrder[3];
            oiio::ImageBufAlgo::channels(tmpBuf, inBuf, 4, &channelOrder.front(), channelValues, {}, false);
            inBuf.swap(tmpBuf);
            // qDebug() << "[QtOIIO] shuffle channels done";

            {
                oiio::ROI exportROI = inBuf.roi();
                exportROI.chbegin = 0;
                exportROI.chend = nchannels;

                // qDebug() << "[QtOIIO] fill output QImage";
                inBuf.get_pixels(exportROI, typeDesc, result.bits());
            }
        }
    }


    if (pixelAspectRatio != 1.0f)
    {
        QSize newSize(inSpec.width * pixelAspectRatio, inSpec.height);
        result = result.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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

        oiio::ImageSpec configSpec;
        configSpec.attribute("raw:user_flip", 1);

        return std::unique_ptr<oiio::ImageInput>(oiio::ImageInput::open(path, &configSpec));
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
        case 5: return QImageIOHandler::TransformationFlipAndRotate90; break;
        case 6: return QImageIOHandler::TransformationRotate90; break;
        case 7: return QImageIOHandler::TransformationMirrorAndRotate90; break;
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
