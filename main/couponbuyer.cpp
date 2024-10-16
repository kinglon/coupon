#include "couponbuyer.h"
#include <Windows.h>

#define SKUID "SK000427"

#define MAX_CANCELE_BUY_TIME 3

CouponBuyer::CouponBuyer(QObject *parent)
    : QObject{parent}
{

}

void CouponBuyer::run()
{
    m_couponBuyStatus.clear();
    for (auto& buyCouponSetting : SettingManager::getInstance()->m_buyCouponSetting)
    {
        if (buyCouponSetting.m_willBuyCount > 0)
        {
            CouponBuyStatus couponBuyStatus;
            couponBuyStatus.m_buyCouponSetting = buyCouponSetting;
            couponBuyStatus.m_needSendWantBuyRequest = true;
        }
    }

    m_mainTimer = new QTimer(this);
    connect(m_mainTimer, &QTimer::timeout, this, &CouponBuyer::onMainTimer);
    m_mainTimer->start(1000);
}

void CouponBuyer::onMainTimer()
{
    // 外部请求取消求购
    if (m_requestStop)
    {
        // 退出前需要取消求购
        bool send = false;
        for (auto& buyStatus : m_couponBuyStatus)
        {
            if (!buyStatus.m_cancelling && !buyStatus.m_recordId.isEmpty() && buyStatus.m_cancelWantBuyTime < MAX_CANCELE_BUY_TIME)
            {
                MfHttpClient* mfClient = new MfHttpClient(this);
                CouponBuyStatus* buyStatusPtr = &buyStatus;
                connect(mfClient, &MfHttpClient::cancelBuyingCompletely, [this, buyStatusPtr, mfClient](bool success, QString errorMsg) {
                    buyStatusPtr->m_cancelling = false;
                    if (!success)
                    {
                        emit printLog(QString::fromWCharArray(L"取消求购失败：%1").arg(buyStatusPtr->m_recordId));
                    }
                    else
                    {
                        buyStatusPtr->m_recordId = "";
                    }
                    mfClient->deleteLater();
                });

                mfClient->cancelBuying(buyStatus.m_recordId);
                buyStatus.m_cancelling = true;
                qInfo("cancel to want buy, record id is %s", buyStatus.m_recordId.toStdString().c_str());
                buyStatus.m_cancelWantBuyTime++;
                send = true;
            }
        }
        if (!send)
        {
            emit runFinish();
        }
        return;
    }

    // 求购
    for (auto& buyStatus : m_couponBuyStatus)
    {
        if (buyStatus.m_needSendWantBuyRequest)
        {
            MfHttpClient* mfClient = new MfHttpClient(this);
            CouponBuyStatus* buyStatusPtr = &buyStatus;
            connect(mfClient, &MfHttpClient::wantBuyCardCompletely, [this, buyStatusPtr, mfClient](bool success, QString errorMsg, QString recordId) {
                if (!success)
                {
                    emit printLog(errorMsg);
                    buyStatusPtr->m_needSendWantBuyRequest = true;
                }
                else
                {
                    QString logContent = QString::fromWCharArray(L"求购面额%1的卡券成功").arg(buyStatusPtr->m_buyCouponSetting.m_faceVal);
                    emit printLog(logContent);
                    buyStatusPtr->m_recordId = recordId;
                }
                mfClient->deleteLater();
            });
            mfClient->wantBuyCard(SKUID, buyStatus.m_buyCouponSetting.m_faceVal,
                                   buyStatus.m_buyCouponSetting.m_willBuyCount,
                                   buyStatus.m_buyCouponSetting.m_discount);
            qInfo("want to buy %d coupons of %d yuan",
                  buyStatus.m_buyCouponSetting.m_willBuyCount,
                  buyStatus.m_buyCouponSetting.m_faceVal);
            buyStatus.m_needSendWantBuyRequest = false;
        }
    }

    // 查询库存
    if (GetTickCount64() - m_lastGetFaceValStockTime >= SettingManager::getInstance()->m_mfQueryStockInterval)
    {
        MfHttpClient* mfClient = new MfHttpClient(this);
        connect(mfClient, &MfHttpClient::getFaceValStockListCompletely, [this, mfClient](bool success, QString errorMsg, QVector<FaceValStock> faceValStocks) {
            if (!success)
            {
                emit printLog(errorMsg);
            }
            else
            {
                for (auto& couponBuyStatus : m_couponBuyStatus)
                {
                    for (auto& faceValStock : faceValStocks)
                    {
                        if (couponBuyStatus.m_buyCouponSetting.m_faceVal == faceValStock.m_faceVal
                                && couponBuyStatus.m_buyCouponSetting.m_discount == faceValStock.m_discount)
                        {
                            QString logContent = QString::fromWCharArray(L"面额%1元有%2张").arg(
                                        faceValStock.m_faceVal, faceValStock.m_count);
                            couponBuyStatus.m_availCount = faceValStock.m_count;
                            break;
                        }
                    }
                }
            }
            mfClient->deleteLater();
        });
        mfClient->getFaceValStockList(SKUID);
        m_lastGetFaceValStockTime = GetTickCount64();
    }

    // 购买卡券
    for (auto& buyStatus : m_couponBuyStatus)
    {
        if (buyStatus.m_availCount > 0 && buyStatus.m_boughtCount < buyStatus.m_buyCouponSetting.m_willBuyCount)
        {
            int buyCount = buyStatus.m_buyCouponSetting.m_willBuyCount - buyStatus.m_boughtCount;
            if (buyStatus.m_availCount < buyCount)
            {
                buyCount = buyStatus.m_availCount;
            }

            MfHttpClient* mfClient = new MfHttpClient(this);
            CouponBuyStatus* buyStatusPtr = &buyStatus;
            connect(mfClient, &MfHttpClient::buyCardCompletely, [this, mfClient, buyStatusPtr](bool success, QString errorMsg, QString recordId) {
                if (!success)
                {
                    emit printLog(errorMsg);
                }
                else
                {
                    buyStatusPtr->m_buyRecordId = recordId;
                }
                mfClient->deleteLater();
            });
            mfClient->buyCard(SKUID, buyStatus.m_buyCouponSetting.m_faceVal, buyCount, buyStatus.m_buyCouponSetting.m_discount);
            qInfo("buy %d coupons of %d yuan", buyCount, buyStatus.m_buyCouponSetting.m_faceVal);
            buyStatus.m_availCount = 0;
        }
    }

    // 获取卡券信息
    for (auto& buyStatus : m_couponBuyStatus)
    {
        if (!buyStatus.m_buyRecordId.isEmpty() && !buyStatus.m_queryCouponInfo)
        {
            MfHttpClient* mfClient = new MfHttpClient(this);
            CouponBuyStatus* buyStatusPtr = &buyStatus;
            connect(mfClient, &MfHttpClient::getCouponCompletely, [this, mfClient, buyStatusPtr](bool success, QString errorMsg, QVector<GetCouponResult> result) {
                buyStatusPtr->m_queryCouponInfo = false;
                if (!success)
                {
                    emit printLog(errorMsg);
                }
                else
                {
                    emit haveNewCoupon(result);
                    buyStatusPtr->m_buyRecordId = "";
                    for (auto& buyStatus : m_couponBuyStatus)
                    {
                        for (auto& coupon : result)
                        {
                            if (buyStatus.m_buyCouponSetting.m_faceVal == coupon.m_coupon.m_faceValue)
                            {
                                buyStatus.m_boughtCount++;
                            }
                        }

                        if (buyStatus.m_boughtCount >= buyStatus.m_buyCouponSetting.m_willBuyCount)
                        {
                            buyStatus.m_boughtCount = 0;
                            buyStatus.m_needSendWantBuyRequest = true;
                        }
                    }
                }
                mfClient->deleteLater();
            });
            mfClient->getCoupon(buyStatus.m_buyRecordId);
            buyStatus.m_queryCouponInfo = true;
        }
    }
}
