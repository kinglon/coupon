#include "logincontroller.h"
#include <Windows.h>
#include <QTimer>
#include "settingmanager.h"
#include "yqbhttpclient.h"

#define REFRESH_INTERVAL (25*60*1000)

LoginController::LoginController(QObject *parent)
    : QObject{parent}
{
    QTimer* mainTimer = new QTimer(this);
    connect(mainTimer, &QTimer::timeout, this, &LoginController::onMainTimer);
    mainTimer->start(5000);
}

void LoginController::onMainTimer()
{
    if (!SettingManager::getInstance()->m_yqbSetting.isValid())
    {
        return;
    }

    if (GetTickCount64() - m_lastRefreshTime < REFRESH_INTERVAL)
    {
        return;
    }

    YqbHttpClient* yqbClient = new YqbHttpClient(this);
    connect(yqbClient, &YqbHttpClient::chargeCompletely, [this](bool success, QString errorMsg, ChargeResult result) {
        if (success)
        {
            qInfo("successful to refresh yqb token");
            m_lastRefreshTime = GetTickCount64();
        }
    });

    const auto& yqbSetting = SettingManager::getInstance()->m_yqbSetting;
    Coupon coupon;
    coupon.m_couponId = yqbSetting.m_couponId;
    coupon.m_couponPassword = yqbSetting.m_couponPassword;
    yqbClient->charge(yqbSetting.m_mobileNumber, coupon, true);
    qInfo("refresh yqb token");
}
