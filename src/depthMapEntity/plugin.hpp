#pragma once

#include "DepthMapEntity.hpp"

#include <QtQml/QtQml>
#include <QtQml/QQmlExtensionPlugin>

namespace depthMapEntity {

class DepthMapEntityQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "depthMapEntity.qmlPlugin")

public:
    void initializeEngine(QQmlEngine* engine, const char* uri) override {}
    void registerTypes(const char* uri) override
    {
        Q_ASSERT(uri == QLatin1String("DepthMapEntity"));
        qmlRegisterType<DepthMapEntity>(uri, 1, 0, "DepthMapEntity");
    }
};

}
