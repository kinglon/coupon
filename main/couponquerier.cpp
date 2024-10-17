#include "couponquerier.h"
#include "yqbhttpclient.h"

CouponQuerier::CouponQuerier(QObject *parent)
    : QObject{parent}
{

}

void CouponQuerier::run(QString mobile, const QVector<Coupon>& coupons)
{
    if (mobile.isEmpty() || coupons.size() == 0)
    {
        qCritical("invalid param");
        emit runFinish(false);
        return;
    }

    m_mobile = mobile;
    m_coupons = coupons;

    doQuery();
}

void CouponQuerier::doQuery()
{
    YqbHttpClient* yqbClient = new YqbHttpClient(this);
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
                result.m_coupon.m_status = result.m_resultMsg;
            }

            emit oneCouponFinish(result.m_coupon);

            m_currentQueryCouponIndex++;
            if (m_currentQueryCouponIndex >= m_coupons.size())
            {
                emit printLog(QString::fromWCharArray(L"查询结束"));
                emit runFinish(true);
            }
            else
            {
                doQuery();
            }
        }
        yqbClient->deleteLater();
    });
    yqbClient->charge(m_mobile, m_coupons[m_currentQueryCouponIndex], true);
}
