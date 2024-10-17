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
    YqbHttpClient* yqbClient = new YqbHttpClient(this);
    connect(yqbClient, &YqbHttpClient::chargeCompletely, [this, yqbClient](bool success, QString errorMsg, ChargeResult result) {
        if (!success)
        {
            // 无法访问壹钱包
            emit printLog(errorMsg);
            emit runFinish(false);
        }
        else
        {
            emit chargeCompletely(result);
            if (!result.m_success)
            {
                emit printLog(result.m_resultMsg);                
            }
            else
            {
                // 充值成功
                m_sumChargeMoney += result.m_realFaceValue;
                QString logContent = QString::fromWCharArray(L"已充值%1").arg(QString::number(m_sumChargeMoney));
                emit printLog(logContent);
                if (m_sumChargeMoney >= m_chargeMoney)
                {
                    emit runFinish(true);
                }
            }

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
    });
    yqbClient->charge(m_mobile, m_coupons[m_currentChargeCouponIndex], false);
}
