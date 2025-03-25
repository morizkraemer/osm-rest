#include "restapi.h"
#include "httpserver.h"
#include "qglobal.h"
#include "qjsonobject.h"
#include "qmetaobject.h"
#include "qnamespace.h"
#include "qobject.h"
#include "qthread.h"
#include "quuid.h"
#include "sourcelist.h"
#include <QDebug>
#include <QJsonDocument>
#include <functional>


namespace  remote {

RestApi::RestApi(SourceList* sourceList, QObject* parent)
    : QObject(parent), m_httpServer(new HttpServer(m_port)), m_httpThread(), m_sourceList(sourceList)
RestApi::RestApi(Settings *settings, SourceList* sourceList, MeterTableModel *meterTable, QObject* parent)
    : QObject(parent), m_httpServer(new HttpServer(m_port)), m_httpThread(), m_sourceList(sourceList), m_meterTable(meterTable), m_settings(settings)
{
    setupRoutes();
    setSettings(m_settings);

}

RestApi::~RestApi() {
    m_httpThread.quit();
    m_httpThread.wait();
    delete m_httpServer;
}

void RestApi::setActive(bool newActive) {
    if (m_active == newActive) return;

    if (newActive) {
        m_httpServer->moveToThread(&m_httpThread);

        m_httpThread.start();

        QMetaObject::invokeMethod(m_httpServer, [=]() {
            m_httpServer->start();
        }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(m_httpServer, "stop", Qt::QueuedConnection);
        m_httpThread.quit();
        m_httpThread.wait();
    }

    m_active = newActive;
    emit activeChanged();
}


void RestApi::setSourceList(SourceList* list) {
    m_sourceList = list;
}

void RestApi::setPort(quint16 newPort){
    m_port = newPort;
    m_httpServer->setPort(newPort);
    emit portChanged(newPort);
}

void RestApi::setSettings(Settings *newSettings)
{
    if (!m_settings) {
        m_settings = newSettings;
    }

    setPort(
        m_settings->reactValue<RestApi, quint16>("port", this, &RestApi::portChanged, port()).toInt()
    );
}

Settings* RestApi::settings() {
    return m_settings;
}

void RestApi::setupRoutes() {
    m_httpServer->registerRoute("GET", "/api/status", [this](const QJsonObject& req) -> QByteArray {
        QJsonObject response;
        response["status"] = "server is running";
        return m_httpServer->buildJsonResponse(200, response);
    });

    m_httpServer->registerRoute("GET", "/api/sources", [this](const QJsonObject& req) -> QByteArray {
        QJsonObject object;
        for (auto& source : m_sourceList->items()) {
            object[source->name()] = source->uuid().toString().remove("{").remove("}");
        };
        return m_httpServer->buildJsonResponse(200, object);
    });

    m_httpServer->registerRoute("GET", "api/meters", [this](const QJsonObject& req) -> QByteArray {
        QJsonObject object;
        for (auto &meter : m_meterTable->getExposedMeters()) {
            object[meter->identifier()] = meter->value();
        }
        return m_httpServer->buildJsonResponse(200, object);
    });
    m_httpServer->registerRoute("GET", "/api/source/{id}", [this](const QJsonObject& req) -> QByteArray {
        QString sourceUUID = req["params"]["id"].toString();
        QJsonObject response;
        auto source = m_sourceList->getByUUid(QUuid::fromString(sourceUUID));
        if (source) {
            const QUuid sourceId = source->uuid();

            QObject* targetQObject = source.get();
            if (targetQObject) {
                QJsonObject object;
                object["api"] = "Open Sound Meter";
                object["version"] = APP_GIT_VERSION;
                object["message"] = "sourceSettings";
                object["uuid"] = sourceId.toString();

                for (int i = 0; i < targetQObject->metaObject()->propertyCount(); ++i) {
                    auto property = targetQObject->metaObject()->property(i);

                    switch (static_cast<int>(property.type())) {

                    case QVariant::Type::Bool:
                        object[property.name()] = property.read(targetQObject).toBool();
                        break;

                    case QVariant::Type::UInt:
                    case QVariant::Type::Int:
                    case QMetaType::Long:
                        object[property.name()] = property.read(targetQObject).toInt();
                        break;

                    case QMetaType::Float:
                        object[property.name()] = property.read(targetQObject).toFloat();
                        break;

                    case QVariant::Type::Double:
                        object[property.name()] = property.read(targetQObject).toDouble();
                        break;

                    case QVariant::Type::String:
                        object[property.name()] = property.read(targetQObject).toString();
                        break;

                    case QVariant::Type::Color: {
                        QJsonObject color;
                        if (source) {
                            color["red"] = source->color().red();
                            color["green"] = source->color().green();
                            color["blue"] = source->color().blue();
                            color["alpha"] = source->color().alpha();
                        }
                        object[property.name()] = color;
                        break;
                    }
                    case QVariant::Type::UserType: {
                        object[property.name()] = property.read(targetQObject).toInt();
                    }
                    default:;
                    }
                }
                return m_httpServer->buildJsonResponse(200, object);
            }
        } else {
            return m_httpServer->buildHttpResponse(400, "text/plain", "source not found");
        }

        return {};
    });


    // TEST ROUTE
    m_httpServer->registerRoute("GET", "/api/test/{test}", [this](const QJsonObject &req) -> QByteArray {
        QJsonObject response;
        response["test"] = req["params"]["test"];
        return m_httpServer->buildJsonResponse(200, response);
    });
};



} //namespace remote
