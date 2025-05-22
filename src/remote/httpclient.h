#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "qhttpmultipart.h"
#include "qjsonobject.h"
#include "qnetworkreply.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qurl.h"

namespace remote {

class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject* parent = nullptr);
    void sendGetRequest(const QUrl& url);
    void sendPostRequest(const QUrl& url, const QJsonObject& json);

private slots:
    void onFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager m_manager;
};
}

#endif
