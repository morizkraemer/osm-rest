#include "httpclient.h"
#include <QDebug>
#include <QJsonDocument>


namespace remote { 

HttpClient::HttpClient(QObject *parent) : QObject(parent) {
}

void HttpClient::sendGetRequest(const QUrl &url) {
    QNetworkRequest request(url);
    m_manager.get(request);
}

void HttpClient::sendPostRequest(const QUrl &url, const QJsonObject &json) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(json);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_manager.post(request, payload);

    // ✅ Handle the reply directly
    connect(reply, &QNetworkReply::finished, reply, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "POST error:" << reply->errorString();
        } else {
            QByteArray response = reply->readAll();
            /*qDebug() << "POST response:" << QString(response);*/
        }
        reply->deleteLater(); // ✅ clean up
    });
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
