#include "httpclientbase.h"
#include <QNetworkProxy>
#include <QNetworkReply>

HttpClientBase::HttpClientBase(QObject *parent)
    : QObject{parent}
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    m_networkAccessManager->setProxy(QNetworkProxy());
    m_networkAccessManager->setTransferTimeout(10*1000);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &HttpClientBase::onHttpFinished);
}

void HttpClientBase::onHttpFinished(QNetworkReply *reply)
{
    onHttpResponse(reply);
    reply->deleteLater();
}

void HttpClientBase::addCommonHeader(QNetworkRequest& request)
{
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");
    request.setRawHeader("Content-Type", "application/json");
}
