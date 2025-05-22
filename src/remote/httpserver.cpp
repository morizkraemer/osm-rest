#include "httpserver.h"
#include "qglobal.h"
#include "qjsonobject.h"
#include "qlist.h"
#include "qmap.h"
#include "qobject.h"
#include "qtcpsocket.h"
#include <QJsonDocument>
#include <functional>

namespace remote {

HttpServer::HttpServer(quint16 port, QObject *parent) : QObject(parent), m_tcpServer(nullptr), m_port(port) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (m_tcpServer) stop();

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &HttpServer::handleNewConnection);

    return m_tcpServer->listen(QHostAddress::Any, m_port);
}

void HttpServer::stop() {
    if (m_tcpServer) {
        m_tcpServer->close();
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
    }
}

quint16 HttpServer::port() const {
    return m_port;
}

void HttpServer::setPort(quint16& newPort) {
    if (m_port != newPort) {
        m_port = newPort;
        emit portChanged();
    }
}

void HttpServer::handleNewConnection() {
    QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, [=]() {
        QByteArray requestData = clientSocket->readAll();
        QString requestString = QString::fromUtf8(requestData);
        QByteArray response = handleHttpRequest(requestString);
        clientSocket->write(response);
        clientSocket->flush();
        clientSocket->waitForBytesWritten(3000);
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    });
}

void HttpServer::registerRoute(const QString &method, const QString &path,
                               std::function<QByteArray(const QJsonObject &)> callback) {
    routes[method + " " + path] = callback;
}

QByteArray HttpServer::buildHttpResponse(int statusCode, const QString &contentType, const QByteArray &body) {
    QByteArray response;
    response.append("HTTP/1.1 " + QByteArray::number(statusCode) + " \r\n");
    if (contentType.isEmpty()){
        response.append("Content-Type: " + contentType.toUtf8() + "\r\n");
        response.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    }
    response.append("Connection: close\r\n");
    response.append("\r\n");
    if (!body.isEmpty()) {
        response.append(body);

    }
    return response;
}

QByteArray HttpServer::buildJsonResponse(int statusCode, const QJsonObject &json) {
    return buildHttpResponse(statusCode, "application/json", QJsonDocument(json).toJson());
}

QByteArray HttpServer::buildStatusResponse(int statusCode) {
    return buildHttpResponse(statusCode, "", nullptr);
}

QByteArray HttpServer::buildErrorResponse(int statusCode, const QString &errorMessage) {
    QJsonObject obj;
    obj["errorMessage"] = errorMessage;
    return buildJsonResponse(statusCode, obj);
}

QByteArray HttpServer::handleHttpRequest(const QString &requestString) {
    QJsonObject requestObject = parseRequest(requestString);
    QString method = requestObject["Method"].toString();
    QString path = requestObject["Path"].toString();
    for (const QString &key : routes.keys()) {
        qDebug() << key;
    }

    QString routeKey = method + " " + path;
    if (routes.contains(routeKey)) {
        return routes[routeKey](requestObject);
    }

    for (auto it = routes.begin(); it != routes.end(); ++it) {
        QString registeredKey = it.key();

        int spaceIndex = registeredKey.indexOf(' ');
        if (spaceIndex == -1) continue;

        QString regMethod = registeredKey.left(spaceIndex);
        QString regPath = registeredKey.mid(spaceIndex + 1);

        if (regMethod != method) continue;

        if (isRouteMatch(regPath, path)) {
            requestObject["params"] = extractRouteParams(regPath, path);
            return it.value()(requestObject);
        }
    }

    return buildHttpResponse(404, "text/plain", "Error: Not Found");
}

bool HttpServer::isRouteMatch(const QString &registeredRoute, const QString &incomingRoute) {
    QStringList registeredParts = registeredRoute.split("/", Qt::SkipEmptyParts);
    QStringList incomingParts = incomingRoute.split("/", Qt::SkipEmptyParts);

    if (registeredParts.size() != incomingParts.size()) {
        return false;
    }

    for (int i = 0; i < registeredParts.size(); ++i) {
        if (registeredParts[i].startsWith("{") && registeredParts[i].endsWith("}")) {
            continue;
        }
        if (registeredParts[i] != incomingParts[i]) {
            return false;
        }
    }
    return true;
}

QJsonObject HttpServer::extractRouteParams(const QString &registeredRoute, const QString &incomingRoute) {
    QJsonObject params;
    QStringList registeredParts = registeredRoute.split("/");
    QStringList incomingParts = incomingRoute.split("/");

    for (int i = 0; i < registeredParts.size(); ++i) {
        if (registeredParts[i].startsWith("{") && registeredParts[i].endsWith("}")) {
            QString paramName = registeredParts[i].mid(1, registeredParts[i].length() - 2);
            params[paramName] = incomingParts[i];
        }
    }
    qDebug() << params;
    return params;
}

QJsonObject HttpServer::parseRequest(const QString &requestString) {
    QJsonObject jsonRequest;

    QStringList rp = requestString.split("\r\n\r\n");
    if (rp.size() > 1) {
        jsonRequest["body"] = rp[1];
    }

    QStringList hd = rp[0].split("\r\n");
    for (int i = 0; i < hd.size(); i++) {
        QString item = hd[i];
        if (i == 0) {
            QStringList f = item.split(" ");
            jsonRequest["Method"] = f[0];
            jsonRequest["Path"] = f[1];
            jsonRequest["Http-Version"] = f[2];
            continue;
        }
        QStringList it = item.split(": ", Qt::SkipEmptyParts);

        if (it.size() >= 1) {
            QString headerKey = it[0].trimmed();
            QString headerValue = (it.size() > 1) ? it[1].trimmed() : "";

            if (jsonRequest.contains(headerKey)) {
                jsonRequest[headerKey] = jsonRequest[headerKey].toString() + ", " + headerValue;
            } else {
                jsonRequest[headerKey] = headerValue;
            }
        }
    }
    return jsonRequest;
}
} // namespace remote
