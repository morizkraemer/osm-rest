#ifndef RESTAPI_H
#define RESTAPI_H

#include "httpserver.h"
#include "qjsonobject.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qthread.h"
#include "sourcelist.h"
#include <QObject>

namespace remote {

class RestApi : public QObject {
    Q_OBJECT
    public:
    explicit RestApi(SourceList *sourceList, QObject *parent = nullptr);
    ~RestApi();

    void Q_INVOKABLE start();
    void Q_INVOKABLE stop();
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    quint16 port() const { return m_port; };
    void setPort(quint16 newPort);


    void setupRoutes();
    void setSourceList(SourceList *list);

    bool active() const { return m_active; };
    void setActive(bool newActive);

signals:
    void portChanged();
    void activeChanged();

private:
    quint16 m_port = 49008;
    HttpServer *m_httpServer;
    QThread m_httpThread;
    bool m_active = false;
    SourceList *m_sourceList;
    /*Settings m_settings; TODO: Saving the port! */
};

} //namespace remote
#endif
