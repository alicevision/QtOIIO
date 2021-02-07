#include "QtOIIOPlugin.hpp"
#include "QtOIIOHandler.hpp"

#include <QImageIOHandler>
#include <QFileDevice>
#include <QDebug>

#include <OpenImageIO/imageio.h>

#include <iostream>

namespace oiio = OIIO;

QtOIIOPlugin::QtOIIOPlugin()
{
    qDebug() << "[QtOIIO] init supported extensions.";

    std::string extensionsListStr;
    oiio::getattribute("extension_list", extensionsListStr);

    QString extensionsListQStr(QString::fromStdString(extensionsListStr));

    QStringList formats = extensionsListQStr.split(';');
    for(auto& format: formats)
    {
        qDebug() << "[QtOIIO] format: " << format << ".";
        QStringList keyValues = format.split(":");
        if(keyValues.size() != 2)
        {
            qDebug() << "[QtOIIO] warning: split OIIO keys: " << keyValues.size() << " for " << format << ".";
        }
        else
        {
            _supportedExtensions += keyValues[1].split(",");
        }
    }
    qDebug() << "[QtOIIO] supported extensions: " << _supportedExtensions.join(", ");
    qInfo() << "[QtOIIO] Plugin Initialized";
}

QtOIIOPlugin::~QtOIIOPlugin()
{
}

QImageIOPlugin::Capabilities QtOIIOPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(!d)
        return QImageIOPlugin::Capabilities();

    const std::string path = d->fileName().toStdString();
    if(path.empty() || path[0] == ':')
        return QImageIOPlugin::Capabilities();

#ifdef QTOIIO_USE_FORMATS_BLACKLIST
    // For performance sake, let Qt handle natively some formats.
    static const QStringList blacklist{"jpeg", "jpg", "png", "ico"};
    if(blacklist.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        return QImageIOPlugin::Capabilities();
    }
#endif
    if (_supportedExtensions.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        qDebug() << "[QtOIIO] Capabilities: extension \"" << QString(format) << "\" supported.";
//        oiio::ImageOutput *out = oiio::ImageOutput::create(path); // TODO: when writting will be implemented
        Capabilities capabilities(CanRead);
//        if(out)
//            capabilities = Capabilities(CanRead|CanWrite);
//        oiio::ImageOutput::destroy(out);
        return capabilities;
    }
    qDebug() << "[QtOIIO] Capabilities: extension \"" << QString(format) << "\" not supported";
    return QImageIOPlugin::Capabilities();
}

QImageIOHandler *QtOIIOPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QtOIIOHandler *handler = new QtOIIOHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
