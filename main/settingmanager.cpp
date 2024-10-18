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

    if (root.contains("buy_coupon"))
    {
        QJsonArray buyCouponSettings = root["buy_coupon"].toArray();
        for (auto item : buyCouponSettings)
        {
            QJsonObject buyCouponSettingJson = item.toObject();
            BuyCouponSetting buyCouponSetting;
            buyCouponSetting.m_faceVal = buyCouponSettingJson["face_val"].toInt();
            buyCouponSetting.m_willBuyCount = buyCouponSettingJson["will_buy_count"].toInt();
            buyCouponSetting.m_discount = buyCouponSettingJson["discount"].toInt();
            m_buyCouponSetting.append(buyCouponSetting);
        }
    }

    if (root.contains("charge_phone"))
    {
        QJsonArray chargePhones = root["charge_phone"].toArray();
        for (auto item : chargePhones)
        {
            QJsonObject chargePhoneJson = item.toObject();
            ChargePhone chargePhone;
            chargePhone.m_id = chargePhoneJson["id"].toString();
            chargePhone.m_moneyCount = chargePhoneJson["money"].toInt();
            chargePhone.m_phoneNumber = chargePhoneJson["phone_number"].toString();
            chargePhone.m_priority = chargePhoneJson["priority"].toInt();
            chargePhone.m_remark = chargePhoneJson["remark"].toInt();
            m_chargePhones.append(chargePhone);
        }
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

    QJsonArray buyCouponArray;
    for (const auto& setting : m_buyCouponSetting)
    {
        QJsonObject buyCouponObject;
        buyCouponObject["face_val"] = setting.m_faceVal;
        buyCouponObject["will_buy_count"] = setting.m_willBuyCount;
        buyCouponObject["discount"] = setting.m_discount;
        buyCouponArray.append(buyCouponObject);
    }
    root["buy_coupon"] = buyCouponArray;

    QJsonArray chargePhoneArray;
    for (const auto& chargePhone : m_chargePhones)
    {
        QJsonObject chargePhoneObject;
        chargePhoneObject["id"] = chargePhone.m_id;
        chargePhoneObject["money"] = chargePhone.m_moneyCount;
        chargePhoneObject["phone_number"] = chargePhone.m_phoneNumber;
        chargePhoneObject["priority"] = chargePhone.m_priority;
        chargePhoneObject["remark"] = chargePhone.m_remark;
        chargePhoneArray.append(chargePhoneObject);
    }
    root["charge_phone"] = chargePhoneArray;

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

void SettingManager::updateChargePhone(const ChargePhone& chargePhone)
{
    for (auto& phone : m_chargePhones)
    {
        if (phone.m_id == chargePhone.m_id)
        {
            phone = chargePhone;
            save();
            return;
        }
    }

    m_chargePhones.append(chargePhone);
    save();
}

void SettingManager::deleteChargePhone(QString id)
{
    for (int i=0; i<m_chargePhones.size(); i++)
    {
        if (m_chargePhones[i].m_id == id)
        {
            m_chargePhones.remove(i);
            break;
        }
    }
    save();
}

int SettingManager::getTotalChargeMoney()
{
    int total = 0;
    for (int i=0; i<m_chargePhones.size(); i++)
    {
        int needChargeMoney = m_chargePhones[i].m_moneyCount - m_chargePhones[i].m_chargeMoney;
        if (needChargeMoney > 0)
        {
            total += needChargeMoney;
        }
    }
    return total;
}
