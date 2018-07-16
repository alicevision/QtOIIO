#include "QtOIIOPlugin.hpp"
#include "QtOIIOHandler.hpp"

#include <qimageiohandler.h>
#include <QFileDevice>
#include <QDebug>

#include <OpenImageIO/imageio.h>

#include <iostream>

namespace oiio = OIIO;

QtOIIOPlugin::QtOIIOPlugin()
{
    qInfo() << "[QtOIIO] init supported extensions.";

    std::string extensionsListStr;
    oiio::getattribute("extension_list", extensionsListStr);

    QString extensionsListQStr(QString::fromStdString(extensionsListStr));
    // qInfo() << "[QtOIIO] extensionsListStr: " << extensionsListQStr << ".";

    QStringList formats = extensionsListQStr.split(';');
    for(auto& format: formats)
    {
        qInfo() << "[QtOIIO] format: " << format << ".";
        QStringList keyValues = format.split(":");
        if(keyValues.size() != 2)
        {
            qInfo() << "[QtOIIO] warning: split OIIO keys: " << keyValues.size() << " for " << format << ".";
        }
        else
        {
            _supportedExtensions += keyValues[1].split(",");
        }
    }
    qInfo() << "[QtOIIO] supported extensions: " << _supportedExtensions.join(", ");
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
        qInfo() << "[QtOIIO] Capabilities: extension \"" << QString(format) << "\" supported.";
//        oiio::ImageOutput *out = oiio::ImageOutput::create(path); // TODO: when writting will be implemented
        Capabilities capabilities(CanRead);
//        if(out)
//            capabilities = Capabilities(CanRead|CanWrite);
//        oiio::ImageOutput::destroy(out);
        return capabilities;
    }
    qInfo() << "[QtOIIO] Capabilities: extension \"" << QString(format) << "\" not supported";
    return 0;
}

QImageIOHandler *QtOIIOPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QtOIIOHandler *handler = new QtOIIOHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
