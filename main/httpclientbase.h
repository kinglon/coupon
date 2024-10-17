#ifndef HTTPCLIENTBASE_H
#define HTTPCLIENTBASE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class HttpClientBase : public QObject
{
    Q_OBJECT
public:
    explicit HttpClientBase(QObject *parent = nullptr);

public:
    // 设置网络请求超时时间，默认是10秒钟
    void setTransferTimeout(int timeOutMs) { m_networkAccessManager->setTransferTimeout(timeOutMs); }

protected:
    virtual void onHttpResponse(QNetworkReply *reply) = 0;

    void addCommonHeader(QNetworkRequest& request);

signals:

private slots:
    void onHttpFinished(QNetworkReply *reply);

protected:
    QNetworkAccessManager* m_networkAccessManager = nullptr;
};

#endif // HTTPCLIENTBASE_H
