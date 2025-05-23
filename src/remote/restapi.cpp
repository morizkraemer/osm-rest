#include "restapi.h"
#include "httpclient.h"
#include "httpserver.h"
#include "meterplot.h"
#include "metertablemodel.h"
#include "qglobal.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qmetaobject.h"
#include "qnamespace.h"
#include "qobject.h"
#include "qthread.h"
#include "qurl.h"
#include "quuid.h"
#include "sourcelist.h"
#include <QDebug>
#include <QJsonDocument>
#include <functional>


namespace  remote {

RestApi::RestApi(Settings *settings, SourceList* sourceList, MeterTableModel *meterTable, QObject* parent)
    : QObject(parent), m_httpServer(new HttpServer(m_port)), m_httpThread(), m_sourceList(sourceList),  m_meterTable(meterTable), m_settings(settings), m_httpClient()
{
    setupRoutes();
    setSettings(m_settings);
    if (startup()) setActive(true);
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
        m_httpClient.moveToThread(&m_httpThread);

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

void RestApi::setStartup(bool newStartup) {
    m_startup = newStartup;
    emit startupChanged(newStartup);
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

    setStartup(
        m_settings->reactValue<RestApi, bool>("startup", this, &RestApi::startupChanged, startup()).toBool()
    );
}

Settings* RestApi::settings() {
    return m_settings;
}

void RestApi::exposeMeters() {
    if (!m_subscribedUrl.isValid()) return;

    for (auto &meter : m_meterTable->getExposedMeters()) {
        auto meterPtr = meter.get();
        connect(meterPtr, &Chart::MeterPlot::valueChanged, this, [this, meterPtr]() {
            QJsonObject obj;
            obj["value"] = meterPtr->value();
            obj["unit"] = meterPtr->modeName();
            obj["type"] = meterPtr->typeName();
            obj["time"] = meterPtr->timeName();
            obj["curve"] = meterPtr->curveName();
            obj["source"] = meterPtr->sourceName();
            m_latestMeterValues[meterPtr->identifier()] = obj;

            if (!m_sendPending) {
                m_sendPending = true;
                QTimer::singleShot(0, this, [this]() {
                    QJsonObject root;
                    QJsonArray data;

                    for (auto it = m_latestMeterValues.begin(); it != m_latestMeterValues.end(); ++it) {
                        QJsonObject obj = it.value();
                        obj["id"] = it.key();
                        data.append(obj);
                    }

                    root["data"] = data;
                    qDebug().noquote() << QJsonDocument(root).toJson(QJsonDocument::Indented);
                    m_httpClient.sendPostRequest(m_subscribedUrl, root, [this](bool success) {
                        if (!success) {
                            m_failedAttempts++;
                            qDebug() << m_failedAttempts;

                            if (m_failedAttempts >= MAX_FAILED_ATTEMPTS) {
                                for (auto &meter : m_meterTable->getExposedMeters()) {
                                    disconnect(meter.get(), nullptr, this, nullptr);
                                }
                                m_subscribedUrl.clear();
                            }
                        } else {
                            m_failedAttempts = 0;
                        }
                    });
                    m_sendPending = false;
                });
            }
        });

}
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

    m_httpServer->registerRoute("GET", "/", [this](const QJsonObject&) -> QByteArray {
        QFile file("../../docs/api.html");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return m_httpServer->buildHttpResponse(404, "text/plain", "File not found");

        QByteArray htmlContent = file.readAll();
        return m_httpServer->buildHttpResponse(200, "text/html", htmlContent);
    });

    m_httpServer->registerRoute("POST", "/api/meters/subscribe", [this](const QJsonObject& req) -> QByteArray {

        QJsonParseError err;
        QJsonObject bodyObj = QJsonDocument::fromJson(req["body"].toString().toUtf8(), &err).object();

        if (err.error != QJsonParseError::NoError) {
            m_httpServer->buildErrorResponse(400, "Invalid Json");
        }

        if (!bodyObj.contains("url")) {
            return m_httpServer->buildErrorResponse(400, "need to specify url");
        }

        QUrl url(bodyObj["url"].toString(), QUrl::StrictMode);

        if (url.isValid() && !url.isRelative()) {
            m_subscribedUrl = url;
            exposeMeters();
            return m_httpServer->buildStatusResponse(200);
        } else {
            return m_httpServer->buildErrorResponse(400, "invalid url specified: " + url.toString());
        }
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
    m_httpServer->registerRoute("GET", "/api/test", [this](const QJsonObject &req) -> QByteArray {
        m_httpClient.sendGetRequest(QUrl("http://localhost:8089/api"));
        return m_httpServer->buildStatusResponse(200);
    });
};



} //namespace remote
