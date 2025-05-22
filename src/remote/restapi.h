#ifndef RESTAPI_H
#define RESTAPI_H

#include "httpclient.h"
#include "httpserver.h"
#include "metertablemodel.h"
#include "qjsonobject.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qqmlproperty.h"
#include "qthread.h"
#include "qurl.h"
#include "settings.h"
#include "sourcelist.h"
#include <QObject>

namespace remote {

class RestApi : public QObject {
    Q_OBJECT
    public:
    explicit RestApi(Settings *settings, SourceList *sourceList, MeterTableModel *meterTable, QObject *parent = nullptr);
    ~RestApi();

    void Q_INVOKABLE start();
    void Q_INVOKABLE stop();
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(Settings *settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(bool startup READ startup WRITE setStartup NOTIFY startupChanged)
    quint16 port() const { return m_port; };
    void setPort(quint16 newPort);


    void setupRoutes();
    void exposeMeters();

    QUrl subscribedUrl() { return m_subscribedUrl; };
    void setSubscribedUrl(QString url);

    void setSourceList(SourceList *list);

    Settings* settings();
    void setSettings(Settings *newSettings);

    bool active() const { return m_active; };
    void setActive(bool newActive);

    bool startup() const { return m_startup; }
    void setStartup(bool newStartup);

signals:
    void portChanged(quint16 port);
    void activeChanged();
    void settingsChanged();
    void startupChanged(bool newStartup);

private:
    quint16 m_port = 49008;
    HttpServer *m_httpServer;
    QThread m_httpThread;
    bool m_active = false;
    bool m_startup = false;
    SourceList *m_sourceList;
    MeterTableModel *m_meterTable;
    Settings *m_settings;
    QUrl m_subscribedUrl;
    HttpClient m_httpClient;
    QMap<QString, QJsonObject> m_latestMeterValues;
    bool m_sendPending = false;
};

} //namespace remote
#endif
