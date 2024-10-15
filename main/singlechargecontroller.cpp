#include "singlechargecontroller.h"
#include "yqbhttpclient.h"

SingleChargeController::SingleChargeController(QObject *parent)
    : QObject{parent}
{

}

void SingleChargeController::run(QString mobile, int chargeMoney, const QVector<Coupon>& coupons)
{
    if (mobile.isEmpty() || chargeMoney <= 0 || coupons.size() == 0)
    {
        qCritical("invalid param");
        emit runFinish(false);
        return;
    }

    m_mobile = mobile;
    m_chargeMoney = chargeMoney;
    m_coupons = coupons;

    doCharge();
}

void SingleChargeController::doCharge()
{
    YqbHttpClient* yqbClient = new YqbHttpClient();
    connect(yqbClient, &YqbHttpClient::chargeCompletely, [this, yqbClient](bool success, QString errorMsg, ChargeResult result) {
        if (!success)
        {
            emit printLog(errorMsg);
            emit runFinish(false);
        }
        else
        {
            if (!result.m_success)
            {
                emit printLog(result.m_resultMsg);
                emit runFinish(false);
            }
            else
            {
                // 充值成功
                emit couponStatusChange(result.m_coupon.m_couponPassword, QString::fromWCharArray(L"已使用"));
                m_sumChargeMoney += result.m_realFaceValue;
                QString logContent = QString::fromWCharArray(L"已充值%1").arg(QString::number(m_sumChargeMoney));
                emit printLog(logContent);
                if (m_sumChargeMoney >= m_chargeMoney)
                {
                    emit runFinish(true);
                }
                else
                {
                    m_currentChargeCouponIndex++;
                    if (m_currentChargeCouponIndex >= m_coupons.size())
                    {
                        emit printLog(QString::fromWCharArray(L"卡券已用完"));
                        emit runFinish(false);
                    }
                    else
                    {
                        doCharge();
                    }
                }

                yqbClient->deleteLater();
            }
        }
    });
    yqbClient->charge(m_mobile, m_coupons[m_currentChargeCouponIndex], false);
}
