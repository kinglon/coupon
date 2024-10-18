#include "mfhttpclient.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include "settingmanager.h"

#define MF_HOST "https://merchant.task.mf178.cn"
#define URI_WANT_BUY_CARD "/api/card/want_buy_card"
#define URI_CANCEL_BUYING "/api/card/cancel_buying"
#define URI_FACEVAL_STOCK_LIST "/api/card/faceval_stock_list"
#define URI_BUY_CARD "/api/card/buy_card_stock"
#define URI_GET_COUPON "/api/card/order_detail"
#define URI_REPORT_ERROR "/api/card/order_deal"

MfHttpClient::MfHttpClient(QObject *parent)
    : HttpClientBase{parent}
{

}

void MfHttpClient::appendPublicParams(QJsonObject& params)
{
    params["app_key"] = SettingManager::getInstance()->m_mfSetting.m_appKey;
    params["timestamp"] = QDateTime::currentDateTime().currentSecsSinceEpoch();
    QStringList keys = params.keys();
    keys.sort();
    QString paramString;
    for (auto& key : keys)
    {
        if (key == "sign" || key == "datas")
        {
            continue;
        }

        if (params[key].isDouble())
        {
            paramString += key + QString::number(params[key].toInt());
        }
        else if (params[key].isString())
        {
            paramString += key + params[key].toString();
        }
        else
        {
            qCritical("the type of param key(%s) is not int or string", key.toStdString().c_str());
        }
    }
    paramString += SettingManager::getInstance()->m_mfSetting.m_appSecret;

    QByteArray textBytes = paramString.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(textBytes);
    params["sign"] = QString(hash.result().toHex());
}

void MfHttpClient::onHttpResponse(QNetworkReply *reply)
{
    QString url = reply->request().url().toString();
    if (url.indexOf(URI_WANT_BUY_CARD) >= 0)
    {
        processWantBuyCardResponse(reply);
    }
    else if (url.indexOf(URI_CANCEL_BUYING) >= 0)
    {
        processCancelBuyingResponse(reply);
    }
    else if (url.indexOf(URI_FACEVAL_STOCK_LIST) >= 0)
    {
        processGetFaceValStockListResponse(reply);
    }
    else if (url.indexOf(URI_GET_COUPON) >= 0)
    {
        processGetCouponResponse(reply);
    }
    else if (url.indexOf(URI_BUY_CARD) >= 0)
    {
        processBuyCardResponse(reply);
    }
    else if (url.indexOf(URI_REPORT_ERROR) >= 0)
    {
        processReportErrorResponse(reply);
    }
}

void MfHttpClient::wantBuyCard(QString sku, int faceVal, int buyCount, int discount)
{
    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_WANT_BUY_CARD);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["goods_sku"] = sku;
    body["face_val"] = faceVal;
    body["total_num"] = buyCount;
    body["discount"] = discount;
    body["cancel_type"] = 1;
    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::cancelBuying(QString recordId)
{
    bool ok = false;
    int recordIdInt = recordId.toInt(&ok);
    if (!ok)
    {
        qCritical("the recordId(%s) is not an int value", recordId.toStdString().c_str());
        emit cancelBuyingCompletely(false, QString::fromWCharArray(L"取消求购失败"));
        return;
    }

    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_CANCEL_BUYING);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["zx_id"] = recordIdInt;
    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::getFaceValStockList(QString sku)
{
    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_FACEVAL_STOCK_LIST);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["goods_sku"] = sku;
    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::buyCard(QString sku, int faceVal, int buyCount, int discount)
{
    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_BUY_CARD);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["goods_sku"] = sku;
    body["face_val"] = faceVal;
    body["callback_url"] = SettingManager::getInstance()->m_mfSetting.m_callbackHost;

    QJsonArray datas;
    QJsonObject data;
    data["discount"] = discount;
    data["buy_num"] = buyCount;
    datas.append(data);
    body["datas"] = datas;

    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::getCoupon(QString recordId)
{
    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_GET_COUPON);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["zx_id"] = recordId;
    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::reportOrderStatus(const OrderStatus& orderStatus)
{
    QNetworkRequest request;
    QUrl url(QString(MF_HOST) + URI_REPORT_ERROR);
    request.setUrl(url);
    addCommonHeader(request);

    QJsonObject body;
    body["zx_id"] = orderStatus.m_recordId;
    body["order_id"] = orderStatus.m_orderId;
    if (orderStatus.m_success)
    {
        body["deal_type"] = 1;
    }
    else
    {
        body["deal_type"] = 2;
        body["remark"] = orderStatus.m_error;
        body["voucher"] = orderStatus.m_imageUrl;
    }

    appendPublicParams(body);

    QJsonDocument jsonDocument(body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
}

void MfHttpClient::processWantBuyCardResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of wanting buy card, error: %d", reply->error());
        emit wantBuyCardCompletely(false, QString::fromWCharArray(L"求购失败：无法访问蜜蜂系统"), "");
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit wantBuyCardCompletely(false, QString::fromWCharArray(L"求购失败：蜜蜂系统返回数据有误"), "");
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        QString recordId = root["data"].toObject()["record_id"].toString();
        emit wantBuyCardCompletely(true, "", recordId);
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit wantBuyCardCompletely(false, message, "");
        return;
    }
}

void MfHttpClient::processCancelBuyingResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of cancelling buy card, error: %d", reply->error());
        emit cancelBuyingCompletely(false, QString::fromWCharArray(L"取消求购失败：无法访问蜜蜂系统"));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit cancelBuyingCompletely(false, QString::fromWCharArray(L"取消求购失败：蜜蜂系统返回数据有误"));
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        emit cancelBuyingCompletely(true, "");
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit cancelBuyingCompletely(false, message);
        return;
    }
}

void MfHttpClient::processGetFaceValStockListResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of getting faceval stock, error: %d", reply->error());
        emit getFaceValStockListCompletely(false, QString::fromWCharArray(L"查询库存失败：无法访问蜜蜂系统"), QVector<FaceValStock>());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit getFaceValStockListCompletely(false, QString::fromWCharArray(L"查询库存失败：蜜蜂系统返回数据有误"), QVector<FaceValStock>());
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        QVector<FaceValStock> faceValStocks;
        QJsonArray datas = root["data"].toArray();
        for (auto data : datas)
        {
            int faceVal = data.toObject()["face_val"].toInt();
            QJsonArray stockList = data.toObject()["stock_list"].toArray();
            for (auto stock : stockList)
            {
                FaceValStock faceValStock;
                faceValStock.m_faceVal = faceVal;
                faceValStock.m_discount = (int)(stock.toObject()["discount"].toString().toFloat());
                faceValStock.m_count = stock.toObject()["stock"].toInt();
                faceValStocks.append(faceValStock);
            }
        }

        emit getFaceValStockListCompletely(true, "", faceValStocks);
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit getFaceValStockListCompletely(false, message, QVector<FaceValStock>());
        return;
    }
}

void MfHttpClient::processBuyCardResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of buying card, error: %d", reply->error());
        emit buyCardCompletely(false, QString::fromWCharArray(L"购买失败：无法访问蜜蜂系统"));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit buyCardCompletely(false, QString::fromWCharArray(L"购买失败：蜜蜂系统返回数据有误"));
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        QString recordId;
        QJsonObject data = root["data"].toObject();
        QJsonArray recordIds = data["record_ids"].toArray();
        QVector<QString> recordStrings;
        for (auto recordId : recordIds)
        {
            recordStrings.append(QString::number(recordId.toInt()));
        }

        emit buyCardCompletely(true, "", recordStrings);
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit buyCardCompletely(false, message);
        return;
    }
}

void MfHttpClient::processGetCouponResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of getting coupon, error: %d", reply->error());
        emit getCouponCompletely(false, QString::fromWCharArray(L"获取卡券失败：无法访问蜜蜂系统"), QVector<GetCouponResult>());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit getCouponCompletely(false, QString::fromWCharArray(L"获取卡券失败：蜜蜂系统返回数据有误"), QVector<GetCouponResult>());
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        QVector<GetCouponResult> results;
        QJsonArray datas = root["data"].toArray();
        for (auto data : datas)
        {
            GetCouponResult result;
            QJsonObject dataJson = data.toObject();
            result.m_orderId = QString::number((qint64)dataJson["order_id"].toDouble());
            result.m_coupon.m_faceValue = dataJson["face_val"].toInt();
            result.m_coupon.m_couponId = dataJson["card_no"].toString();
            result.m_coupon.m_couponPassword = dataJson["card_pwd"].toString();
            results.append(result);

            qInfo("buy a coupon, order id is %s, face value is %d, coupon id is %s, coupon password is %s",
                  result.m_orderId.toStdString().c_str(), result.m_coupon.m_faceValue,
                  result.m_coupon.m_couponId.toStdString().c_str(),
                  result.m_coupon.m_couponPassword.toStdString().c_str());
        }

        emit getCouponCompletely(true, "", results);
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit getCouponCompletely(false, message, QVector<GetCouponResult>());
        return;
    }
}

void MfHttpClient::processReportErrorResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of reporting error, error: %d", reply->error());
        emit reportOrderStatusCompletely(false, QString::fromWCharArray(L"报告错误失败：无法访问蜜蜂系统"));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit reportOrderStatusCompletely(false, QString::fromWCharArray(L"报告错误失败：蜜蜂系统返回数据有误"));
        return;
    }

    QJsonObject root = jsonDocument.object();
    if (root.contains("code") && root["code"].toInt() == 0)
    {
        emit reportOrderStatusCompletely(true, "");
        return;
    }
    else
    {
        QString message = root["message"].toString();
        emit reportOrderStatusCompletely(false, message);
        return;
    }
}
