#include "yqbhttpclient.h"
#include "settingmanager.h"
#include "Utility/ImPath.h"
#include <QFile>
#include <QJsonDocument>
#include <QDateTime>

#define YQB_HOST "https://mzone.yqb.com"

#define URI_VERIFY_COUPON "/mzone-http/point/ticket_verif_risk"

#define URI_RECHARGE "/mzone-http/point/recharge_risk"

#define MAX_RETRY_COUNT 2

YqbHttpClient::YqbHttpClient(QObject *parent)
    : HttpClientBase{parent}
{

}

void YqbHttpClient::charge(QString mobile, const Coupon& coupon, bool onlyQueryCoupon)
{
    m_onlyQueryCoupon = onlyQueryCoupon;
    m_retryCount = 0;
    m_mobile = mobile;
    m_result.m_coupon = coupon;
    m_result.m_queryDateTime = QDateTime::currentDateTime();
    if (m_mobile.isEmpty() || !m_result.m_coupon.isValid())
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：手机号为空或卡券无效"), ChargeResult());
        return;
    }

    if (SettingManager::getInstance()->m_yqbSetting.m_yqbToken.isEmpty())
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：壹钱包未登录"), ChargeResult());
        return;
    }

    sendVerifyCouponRequest();
}

void YqbHttpClient::sendVerifyCouponRequest()
{
    QNetworkRequest request;
    QUrl url(QString(YQB_HOST) + URI_VERIFY_COUPON + "?m=" + SettingManager::getInstance()->m_yqbSetting.m_yqbToken);
    request.setUrl(url);
    addCommonHeader(request);
    request.setRawHeader("Cookie", (QString("mzone_session_id=") + SettingManager::getInstance()->m_yqbSetting.m_yqbToken).toUtf8());

    QJsonObject* body = getVerifyCouponBodyTemplate();
    if (body == nullptr)
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：无法找到校验卡券模板文件"), ChargeResult());
        return;
    }
    (*body)["rechargeCode"] = m_result.m_coupon.m_couponPassword;
    (*body)["serial"] = m_result.m_coupon.m_couponId.right(4);
    (*body)["account"] = m_mobile;
    QJsonDocument jsonDocument(*body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
    qInfo("send verify coupon request for %s", m_result.m_coupon.m_couponId.toStdString().c_str());
}

QJsonObject* YqbHttpClient::getVerifyCouponBodyTemplate()
{
    static QJsonObject bodyTemplate;
    if (!bodyTemplate.isEmpty())
    {
        return &bodyTemplate;
    }

    std::wstring bodyTemplateFile = CImPath::GetConfPath() + L"verify_coupon_body_template.json";
    QFile file(QString::fromStdWString(bodyTemplateFile));
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical("failed to open the body template of verifying coupon");
        return nullptr;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    bodyTemplate = jsonDocument.object();
    return &bodyTemplate;
}

QJsonObject* YqbHttpClient::getRechargeBodyTemplate()
{
    static QJsonObject bodyTemplate;
    if (!bodyTemplate.isEmpty())
    {
        return &bodyTemplate;
    }

    std::wstring bodyTemplateFile = CImPath::GetConfPath() + L"recharge_body_template.json";
    QFile file(QString::fromStdWString(bodyTemplateFile));
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical("failed to open the body template of recharging");
        return nullptr;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    bodyTemplate = jsonDocument.object();
    return &bodyTemplate;
}

void YqbHttpClient::onHttpResponse(QNetworkReply *reply)
{
    QString url = reply->request().url().toString();
    if (url.indexOf(URI_VERIFY_COUPON) >= 0)
    {
        processVerifyCouponResponse(reply);
    }
    else if (url.indexOf(URI_RECHARGE) >= 0)
    {
        processRechargeResponse(reply);
    }
}

void YqbHttpClient::processVerifyCouponResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of verifying coupon, error: %d", reply->error());
        if (m_retryCount < MAX_RETRY_COUNT)
        {
            m_retryCount++;
            sendVerifyCouponRequest();
        }
        else
        {
            emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：无法访问壹钱包"), ChargeResult());
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：壹钱包返回数据有误"), ChargeResult());
        return;
    }

    QJsonObject root = jsonDocument.object();
    QString resultCode = root["resultCode"].toString();
    if (resultCode == "1000")
    {
        m_verifyCouponResponseData = root["data"].toObject();
        if (m_onlyQueryCoupon)
        {
            m_result.m_success = true;
            m_result.m_coupon.m_faceValue = m_verifyCouponResponseData["point"].toInt() / 500;
            m_result.m_coupon.m_expiredDate = m_verifyCouponResponseData["expiredDate"].toString();
            emit chargeCompletely(true, "", m_result);
            return;
        }
        else
        {
            m_retryCount = 0;
            sendRechargeRequest();
            return;
        }
    }
    else
    {
        m_result.m_success = false;
        m_result.m_resultMsg = root["resultMsg"].toString();
        emit chargeCompletely(true, "", m_result);
        return;
    }
}

void YqbHttpClient::sendRechargeRequest()
{
    QNetworkRequest request;
    QUrl url(QString(YQB_HOST) + URI_RECHARGE + "?m=" + SettingManager::getInstance()->m_yqbSetting.m_yqbToken);
    request.setUrl(url);
    addCommonHeader(request);
    request.setRawHeader("Cookie", (QString("mzone_session_id=") + SettingManager::getInstance()->m_yqbSetting.m_yqbToken).toUtf8());

    QJsonObject* body = getRechargeBodyTemplate();
    if (body == nullptr)
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：无法找到充值模板文件"), ChargeResult());
        return;
    }
    (*body)["account"] = m_mobile;
    (*body)["partnerId"] = m_verifyCouponResponseData["partnerId"].toString();
    (*body)["rechargeCode"] = m_result.m_coupon.m_couponPassword;
    (*body)["riskRequestId"] = m_verifyCouponResponseData["riskRequestId"].toString();
    (*body)["serial"] = m_result.m_coupon.m_couponId;
    (*body)["serialSuffix"] = m_result.m_coupon.m_couponId.right(4);

    QJsonDocument jsonDocument(*body);
    QByteArray jsonData = jsonDocument.toJson();
    m_networkAccessManager->post(request, jsonData);
    qInfo("send recharge request for %s", m_result.m_coupon.m_couponId.toStdString().c_str());
}

void YqbHttpClient::processRechargeResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the request of recharging, error: %d", reply->error());
        if (m_retryCount < MAX_RETRY_COUNT)
        {
            m_retryCount++;
            sendRechargeRequest();
        }
        else
        {
            emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：无法访问壹钱包"), ChargeResult());
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
    if (jsonDocument.isNull() || jsonDocument.isEmpty())
    {
        emit chargeCompletely(false, QString::fromWCharArray(L"充值失败：壹钱包返回数据有误"), ChargeResult());
        return;
    }

    QJsonObject root = jsonDocument.object();
    QString resultCode = root["resultCode"].toString();
    if (resultCode == "1000")
    {
        m_result.m_success = true;
        int point = root["data"].toObject()["point"].toInt();
        m_result.m_realFaceValue = point / 500;
        emit chargeCompletely(true, "", m_result);
        return;
    }
    else
    {
        m_result.m_success = false;
        m_result.m_resultMsg = root["resultMsg"].toString();
        emit chargeCompletely(true, "", m_result);
        return;
    }
}
