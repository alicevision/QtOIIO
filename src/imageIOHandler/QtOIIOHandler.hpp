#pragma once
#include <QImageIOHandler>
#include <QImage>

class QtOIIOHandler : public QImageIOHandler
{
public:
    QtOIIOHandler();
    ~QtOIIOHandler();

    bool canRead() const;
    bool read(QImage *image);
    bool write(const QImage &image);

    QByteArray name() const;

    static bool canRead(QIODevice *device);

    QVariant option(ImageOption option) const;
    void setOption(ImageOption option, const QVariant &value);
    bool supportsOption(ImageOption option) const;
};
