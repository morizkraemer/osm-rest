#ifndef REST_API_H
#define REST_API_H

#include "qbuffer.h"
#include "qchar.h"
#include "qglobal.h"
#include "qjsonobject.h"
#include "qmap.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qtcpserver.h"
#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <functional>

namespace remote {


class HttpServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)

public:
    explicit HttpServer(quint16 port, QObject *parent = nullptr);
    ~HttpServer();

    void registerRoute(const QString &method, const QString &path, std::function<QByteArray(const QJsonObject &)> callback);
    QByteArray handleHttpRequest(const QString &requestString);
    QByteArray buildHttpResponse(int statusCode, const QString &contentType, const QByteArray &body);
    QByteArray buildJsonResponse(int statusCode, const QJsonObject &json);
    QByteArray buildErrorResponse(int statusCode, const QString &errorMessage);
    QByteArray buildStatusResponse(int statusCode);

    quint16 port() const;
    void setPort(quint16& newPort);

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();

    signals:
    void portChanged();

private slots:
    void handleNewConnection();

private:
    QJsonObject parseRequest(const QString &request);
    bool isRouteMatch(const QString &registeredRoute, const QString &incomingRoute);
    QJsonObject extractRouteParams(const QString &registeredRoute, const QString &incomingRoute);

    QTcpServer *m_tcpServer;
    quint16 m_port;

    QMap<QString, std::function<QByteArray(const QJsonObject &)>> routes;
};

} //namespace remote
#endif // REST_API_H
