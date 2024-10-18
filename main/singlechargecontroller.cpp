#include "singlechargecontroller.h"
#include "yqbhttpclient.h"
#include <QTimer>

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
                if (result.m_coupon.m_faceValue == result.m_realFaceValue)
                {
                    QString logContent = QString::fromWCharArray(L"手机%1成功充值%2元").arg(
                                m_mobile, QString::number(result.m_realFaceValue));
                    emit printLog(logContent);
                }
                else
                {
                    QString logContent = QString::fromWCharArray(L"手机%1成功充值%2元，与面值%3元不一致").arg(
                                m_mobile, QString::number(result.m_realFaceValue),
                                QString::number(result.m_coupon.m_faceValue));
                    emit printLog(logContent);
                }
            }

            if (m_sumChargeMoney >= m_chargeMoney)
            {
                emit runFinish(true);
            }
            else
            {
                m_currentChargeCouponIndex++;
                if (m_currentChargeCouponIndex >= m_coupons.size())
                {
                    emit runFinish(false);
                }
                else
                {
                    // 延后2秒继续充值
                    QTimer* timer = new QTimer(this);
                    connect(timer, &QTimer::timeout, [this, timer]() {
                        timer->stop();
                        timer->deleteLater();
                        doCharge();
                    });
                    timer->start(2000);
                }
            }
        }

        yqbClient->deleteLater();
    });
    yqbClient->charge(m_mobile, m_coupons[m_currentChargeCouponIndex], false);
}
