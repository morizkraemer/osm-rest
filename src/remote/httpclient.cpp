#include "httpclient.h"
#include <QDebug>
#include <QJsonDocument>


namespace remote { 

HttpClient::HttpClient(QObject *parent) : QObject(parent) {
    connect(&m_manager, &QNetworkAccessManager::finished, this, &HttpClient::onFinished);
}

void HttpClient::sendGetRequest(const QUrl &url) {
    QNetworkRequest request(url);
    m_manager.get(request);
}

void HttpClient::sendPostRequest(const QUrl &url, const QJsonObject &json) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(json);
    m_manager.post(request, doc.toJson());
}

void HttpClient::onFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error:" << reply->errorString();
    } else {
        QByteArray response = reply->readAll();
        qDebug() << "Response:" << QString(response);
    }
    reply->deleteLater();
} 
} // namespace remote
