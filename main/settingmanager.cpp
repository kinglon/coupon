#include "settingmanager.h"
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/ImCharset.h"
#include "Utility/LogMacro.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SettingManager::SettingManager()
{
    load();
}

SettingManager* SettingManager::getInstance()
{
    static SettingManager* pInstance = new SettingManager();
	return pInstance;
}

void SettingManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs.json";    
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the basic configure file : %s", strConfFilePath.c_str());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();

    if (root.contains("log_level"))
    {
        m_nLogLevel = root["log_level"].toInt();
    }

    if (root.contains("query_stock_interval"))
    {
        m_mfQueryStockInterval = root["query_stock_interval"].toInt();
    }

    if (root.contains("yqb"))
    {
        QJsonObject yqb = root["yqb"].toObject();
        m_yqbSetting.m_yqbToken = yqb["token"].toString();
        m_yqbSetting.m_mobileNumber = yqb["mobile_number"].toString();
        m_yqbSetting.m_couponId = yqb["coupon_id"].toString();
        m_yqbSetting.m_couponPassword = yqb["coupon_password"].toString();
    }        

    if (root.contains("coupon"))
    {
        QJsonArray coupons = root["coupon"].toArray();
        for (auto item : coupons)
        {
            QJsonObject couponJson = item.toObject();
            Coupon coupon;
            coupon.m_couponId = couponJson["coupon_id"].toString();
            coupon.m_couponPassword = couponJson["coupon_password"].toString();
            m_coupons.append(coupon);
        }
    }

    if (root.contains("mf"))
    {
        QJsonObject mfObject = root["mf"].toObject();
        m_mfSetting.m_appKey = mfObject["appkey"].toString();
        m_mfSetting.m_appSecret = mfObject["appsecret"].toString();
        m_mfSetting.m_callbackHost = mfObject["callback_host"].toString();
    }

    if (root.contains("external_path"))
    {
        m_externalPath = root["external_path"].toString();
    }
}

void SettingManager::save()
{
    QJsonObject root;
    root["log_level"] = m_nLogLevel;
    root["query_stock_interval"] = m_mfQueryStockInterval;

    QJsonObject yqb;
    yqb["token"] = m_yqbSetting.m_yqbToken;
    yqb["mobile_number"] = m_yqbSetting.m_mobileNumber;
    yqb["coupon_id"] = m_yqbSetting.m_couponId;
    yqb["coupon_password"] = m_yqbSetting.m_couponPassword;
    root["yqb"] = yqb;    

    QJsonArray couponArray;
    for (const auto& coupon : m_coupons)
    {
        QJsonObject couponObject;
        couponObject["coupon_id"] = coupon.m_couponId;
        couponObject["coupon_password"] = coupon.m_couponPassword;
        couponArray.append(couponObject);
    }
    root["coupon"] = couponArray;

    QJsonObject mfObject;
    mfObject["appkey"] = m_mfSetting.m_appKey;
    mfObject["appsecret"] = m_mfSetting.m_appSecret;
    mfObject["callback_host"] = m_mfSetting.m_callbackHost;
    root["mf"] = mfObject;

    root["external_path"] = m_externalPath;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save setting");
        return;
    }
    file.write(jsonData);
    file.close();
}
