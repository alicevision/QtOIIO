#include "QtOIIOPlugin.hpp"
#include "QtOIIOHandler.hpp"

#include <qimageiohandler.h>
#include <QFileDevice>

#include <OpenImageIO/imageio.h>

#include <iostream>

namespace oiio = OIIO;

QtOIIOPlugin::QtOIIOPlugin()
{
    std::cout << "[QtOIIO] init supported extensions." << std::endl;

    std::string extensionsListStr;
    oiio::getattribute("extension_list", extensionsListStr);

    QString extensionsListQStr(QString::fromStdString(extensionsListStr));

    std::cout << "[QtOIIO] extensionsListQStr: " << extensionsListQStr.toStdString() << "." << std::endl;
    std::cout << "[QtOIIO] extensionsListStr: " << extensionsListStr << "." << std::endl;

    QStringList formats = extensionsListQStr.split(';');
    for(auto& format: formats)
    {
        std::cout << "[QtOIIO] format: " << format.toStdString() << "." << std::endl;
        QStringList keyValues = format.split(":");
        if(keyValues.size() != 2)
        {
            std::cout << "[QtOIIO] warning: split OIIO keys: " << keyValues.size() << " for " << format.toStdString() << "." << std::endl;
        }
        else
        {
            _supportedExtensions += keyValues[1].split(",");
        }
    }
    std::cout << "[QtOIIO] supported extensions: " << _supportedExtensions.join(", ").toStdString() << std::endl;
}

QtOIIOPlugin::~QtOIIOPlugin()
{
}

QImageIOPlugin::Capabilities QtOIIOPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(!d)
        return 0;

    const std::string path = d->fileName().toStdString();
    if(path.empty() || path[0] == ':')
        return 0;

    if (_supportedExtensions.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        std::cout << "[QtOIIO] Capabilities: extension \"" << QString(format).toStdString() << "\" supported." << std::endl;
//        oiio::ImageOutput *out = oiio::ImageOutput::create(path); // TODO: when writting will be implemented
        Capabilities capabilities(CanRead);
//        if(out)
//            capabilities = Capabilities(CanRead|CanWrite);
//        oiio::ImageOutput::destroy(out);
        return capabilities;
    }
    std::cout << "[QtOIIO] Capabilities: extension \"" << QString(format).toStdString() << "\" not supported" << std::endl;
    return 0;
}

QImageIOHandler *QtOIIOPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QtOIIOHandler *handler = new QtOIIOHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
