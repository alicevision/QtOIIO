#pragma once

#include <qimageiohandler.h>
#include <QtCore/QtGlobal>

#include <iostream>

// QImageIOHandlerFactoryInterface_iid

// QT_DECL_EXPORT
class QtOIIOPlugin : public QImageIOPlugin
{
public:
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "QtOIIOPlugin.json")

public:
    QStringList _supportedExtensions;

public:
    QtOIIOPlugin();
    ~QtOIIOPlugin();

    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

