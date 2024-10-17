#include "couponbuyer.h"
#include <Windows.h>

#define SKUID "SK000427"

// 最多取消求购请求次数
#define MAX_CANCELE_BUY_TIME 3

// 求购请求间隔
#define WANT_BUY_REQUEST_INTERVAL 6000

CouponBuyer::CouponBuyer(QObject *parent)
    : QObject{parent}
{

}

void CouponBuyer::run()
{
    m_totalWillBuyMoney = SettingManager::getInstance()->getTotalChargeMoney();

    m_couponBuyStatus.clear();
    for (auto& buyCouponSetting : SettingManager::getInstance()->m_buyCouponSetting)
    {
        if (buyCouponSetting.m_willBuyCount > 0)
        {
            CouponBuyStatus couponBuyStatus;
            couponBuyStatus.m_buyCouponSetting = buyCouponSetting;
            couponBuyStatus.m_needSendWantBuyRequest = true;
            m_couponBuyStatus.append(couponBuyStatus);
        }
    }

    m_mainTimer = new QTimer(this);
    connect(m_mainTimer, &QTimer::timeout, this, &CouponBuyer::onMainTimer);
    m_mainTimer->start(1000);
}

void CouponBuyer::onMainTimer()
{
    // 已达到购买金额，可以退出了
    int needMoney = m_totalWillBuyMoney - m_totalBoughtMoney;
    if (needMoney <= 0)
    {
        m_requestStop = true;
    }

    // 请求取消求购
    if (m_requestStop)
    {
        // 退出前需要取消求购
        bool send = false;
        for (auto& buyStatus : m_couponBuyStatus)
        {
            if (!buyStatus.m_cancelling && !buyStatus.m_recordId.isEmpty()
                    && buyStatus.m_cancelWantBuyTime < MAX_CANCELE_BUY_TIME)
            {
                MfHttpClient* mfClient = new MfHttpClient(this);
                CouponBuyStatus* buyStatusPtr = &buyStatus;
                connect(mfClient, &MfHttpClient::cancelBuyingCompletely, [this, buyStatusPtr, mfClient](bool success, QString errorMsg) {
                    buyStatusPtr->m_cancelling = false;
                    if (success || errorMsg.indexOf(QString::fromWCharArray(L"无法再取消求购")) >= 0)
                    {
                        emit printLog(QString::fromWCharArray(L"取消求购面额%1元成功").arg(QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal)));
                        buyStatusPtr->m_recordId = "";
                    }
                    else
                    {
                        emit printLog(QString::fromWCharArray(L"取消求购面额%1元失败: %2").arg(QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal), errorMsg));
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
        if (GetTickCount64() - m_lastSendWantBuyReqTime >= WANT_BUY_REQUEST_INTERVAL
                && buyStatus.m_needSendWantBuyRequest)
        {
            MfHttpClient* mfClient = new MfHttpClient(this);
            CouponBuyStatus* buyStatusPtr = &buyStatus;
            connect(mfClient, &MfHttpClient::wantBuyCardCompletely, [this, buyStatusPtr, mfClient](bool success, QString errorMsg, QString recordId) {
                if (!success)
                {
                    QString logContent = QString::fromWCharArray(L"面额%1元求购失败：%2").arg(
                                QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal),
                                errorMsg);
                    emit printLog(logContent);
                    // 您有正在求购的订单，请处理完成后再次求购
                    if (errorMsg.indexOf(QString::fromWCharArray(L"有正在求购")) < 0)
                    {
                        buyStatusPtr->m_needSendWantBuyRequest = true;
                    }
                }
                else
                {
                    QString logContent = QString::fromWCharArray(L"求购面额%1元的卡券成功").arg(buyStatusPtr->m_buyCouponSetting.m_faceVal);
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
            m_lastSendWantBuyReqTime = GetTickCount64();
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
                                        QString::number(faceValStock.m_faceVal),
                                        QString::number(faceValStock.m_count));
                            emit printLog(logContent);
                            couponBuyStatus.m_availCount = faceValStock.m_count;
                            break;
                        }
                    }
                }
            }
            mfClient->deleteLater();
        });
        mfClient->getFaceValStockList(SKUID);
        emit printLog(QString::fromWCharArray(L"查询库存"));
        m_lastGetFaceValStockTime = GetTickCount64();
    }

    // 购买卡券    
    for (auto& buyStatus : m_couponBuyStatus)
    {
        if (buyStatus.canBuyCard() && buyStatus.m_buyCouponSetting.m_faceVal <= needMoney)
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
                    QString logContent = QString::fromWCharArray(L"面额%1元购买失败：%2").arg(
                                QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal),
                                errorMsg);
                    emit printLog(logContent);
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
                    QString logContent = QString::fromWCharArray(L"面额%1元卡券信息获取失败：%2").arg(
                                QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal),
                                errorMsg);
                    emit printLog(logContent);
                }
                else
                {
                    if (result.isEmpty())
                    {
                        qInfo("it is buying, not have any coupons now");
                    }
                    else
                    {
                        QString logContent = QString::fromWCharArray(L"面额%1元卡券成功购买%2张").arg(
                                QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal),
                                QString::number(result.size()));
                        emit printLog(logContent);

                        for (auto& item : result)
                        {
                            item.m_recordId = buyStatusPtr->m_buyRecordId;
                            m_totalBoughtMoney += item.m_coupon.m_faceValue;
                        }

                        if (m_totalBoughtMoney < m_totalWillBuyMoney)
                        {
                            logContent = QString::fromWCharArray(L"待购买总金额%1").arg(
                                    QString::number(m_totalWillBuyMoney-m_totalBoughtMoney));
                            emit printLog(logContent);
                        }

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
                }
                mfClient->deleteLater();
            });
            mfClient->getCoupon(buyStatus.m_buyRecordId);
            buyStatus.m_queryCouponInfo = true;
        }
    }
}
