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
    m_totalWillBuyMoney = ChargeSettingManager::getInstance()->getTotalChargeMoney();

    m_couponBuyStatus.clear();
    for (auto& buyCouponSetting : ChargeSettingManager::getInstance()->m_buyCouponSetting)
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
    if (m_requestStop)
    {
        // 如果还有卡券待查询，需要继续查询
        if (needQueryCouponInfo())
        {
            doGetCouponInfo();
            return;
        }

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
                    BuyRecord buyRecord;
                    buyRecord.m_buyRecordId = recordId;
                    buyStatusPtr->m_buyRecords.append(buyRecord);
                    qInfo("add a buy record: %s", recordId.toStdString().c_str());
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
        qInfo("query coupon stock list");
        m_lastGetFaceValStockTime = GetTickCount64();
    }

    // 购买卡券
    for (auto& buyStatus : m_couponBuyStatus)
    {
        if (!buyStatus.canBuyCard())
        {
            continue;
        }

        int buyCount = buyStatus.getCanBuyCount();

        // 不可超过待购买的额度
        int needMoney = m_totalWillBuyMoney - m_totalBoughtMoney;
        int needCount = needMoney / buyStatus.m_buyCouponSetting.m_faceVal;
        if (needCount < buyCount)
        {
            buyCount = needCount;
        }
        if (buyCount <= 0)
        {
            continue;
        }

        MfHttpClient* mfClient = new MfHttpClient(this);
        CouponBuyStatus* buyStatusPtr = &buyStatus;
        connect(mfClient, &MfHttpClient::buyCardCompletely, [this, mfClient, buyStatusPtr, buyCount](bool success, QString errorMsg, QVector<QString> recordIds) {
            if (!success)
            {
                QString logContent = QString::fromWCharArray(L"面额%1元购买失败：%2").arg(
                            QString::number(buyStatusPtr->m_buyCouponSetting.m_faceVal),
                            errorMsg);
                emit printLog(logContent);
            }
            else
            {
                if (recordIds.size() != 1)
                {
                    qCritical("buy card completely, but the size of record is not 1");
                }
                else
                {
                    onBuyCoupon(buyStatusPtr, recordIds[0], buyCount);
                }
            }

            mfClient->deleteLater();
        });
        mfClient->buyCard(SKUID, buyStatus.m_buyCouponSetting.m_faceVal, buyCount, buyStatus.m_buyCouponSetting.m_discount);
        qInfo("buy %d coupons of %d yuan", buyCount, buyStatus.m_buyCouponSetting.m_faceVal);
        buyStatus.m_availCount -= buyCount;
    }

    // 查询卡券信息
    doGetCouponInfo();
}

void CouponBuyer::onBuyCoupon(CouponBuyStatus* buyStatus, QString recordId, int buyCount)
{
    // 更新累计已购买金额
    m_totalBoughtMoney += buyStatus->m_buyCouponSetting.m_faceVal * buyCount;
    if (m_totalBoughtMoney < m_totalWillBuyMoney)
    {
        QString logContent = QString::fromWCharArray(L"待购买总金额%1").arg(
                QString::number(m_totalWillBuyMoney-m_totalBoughtMoney));
        emit printLog(logContent);
    }

    // 更新已购买张数，如果消耗完重新发起求购
    buyStatus->addBuyCount(recordId, buyCount);
    if (recordId == buyStatus->m_recordId)
    {
        for (auto& buyRecord : buyStatus->m_buyRecords)
        {
            if (buyRecord.m_buyRecordId == recordId)
            {
                if (buyRecord.m_boughtCount >= buyStatus->m_buyCouponSetting.m_willBuyCount)
                {
                    buyStatus->m_needSendWantBuyRequest = true;
                }
                break;
            }
        }
    }
}

bool CouponBuyer::needQueryCouponInfo()
{
    for (auto& buyStatus : m_couponBuyStatus)
    {
        for (auto& buyRecord : buyStatus.m_buyRecords)
        {
            if (buyRecord.m_orderIds.size() < buyRecord.m_boughtCount)
            {
                return true;
            }
        }
    }

    return false;
}

void CouponBuyer::doGetCouponInfo()
{
    for (auto& buyStatus : m_couponBuyStatus)
    {
        for (auto& buyRecord : buyStatus.m_buyRecords)
        {
            if (!buyRecord.needQueryCouponInfo())
            {
                continue;
            }

            MfHttpClient* mfClient = new MfHttpClient(this);
            CouponBuyStatus* buyStatusPtr = &buyStatus;
            QString buyRecordId = buyRecord.m_buyRecordId;
            connect(mfClient, &MfHttpClient::getCouponCompletely, [this, mfClient, buyStatusPtr, buyRecordId](bool success, QString errorMsg, QVector<GetCouponResult> result) {
                onGetCouponCompletely(buyStatusPtr, buyRecordId, success, errorMsg, result);
                mfClient->deleteLater();
            });
            mfClient->getCoupon(buyRecordId);
            buyRecord.m_queryCouponInfo = true;
        }
    }
}

void CouponBuyer::onGetCouponCompletely(CouponBuyStatus* buyStatus, QString buyRecordId, bool success, QString errorMsg, QVector<GetCouponResult> result)
{
    BuyRecord* buyRecord = buyStatus->getBuyRecord(buyRecordId);
    if (buyRecord == nullptr)
    {
        qCritical("failed to find buy record of %s", buyRecordId.toStdString().c_str());
        return;
    }

    buyRecord->m_queryCouponInfo = false;

    if (!success)
    {
        QString logContent = QString::fromWCharArray(L"卡券信息获取失败：%1").arg(errorMsg);
        emit printLog(logContent);

        // order_id或zx_id必须传一个，不再获取
        if (errorMsg.indexOf(QString::fromWCharArray(L"必须传一个")) >= 0)
        {
            buyRecord->m_allowQueryCouponInfo = false;
        }
    }
    else
    {
        QVector<GetCouponResult> coupons;
        for (auto& item : result)
        {
            if (!buyRecord->m_orderIds.contains(item.m_orderId))
            {
                GetCouponResult coupon = item;
                coupon.m_recordId = buyRecordId;
                coupons.append(coupon);
                buyRecord->m_orderIds.append(coupon.m_orderId);
            }
        }

        // 如果卡券全部获取到，可以删除掉
        if (buyRecord->m_orderIds.size() >= buyStatus->m_buyCouponSetting.m_willBuyCount)
        {
            for (int i=0; i<buyStatus->m_buyRecords.size(); i++)
            {
                if (buyStatus->m_buyRecords[i].m_buyRecordId == buyRecordId)
                {
                    buyStatus->m_buyRecords.remove(i);
                    qInfo("delete a buy record: %s", buyRecordId.toStdString().c_str());
                    break;
                }
            }
        }

        if (coupons.isEmpty())
        {
            qInfo("it is buying, not have any coupons now");
        }
        else
        {

            QString logContent = QString::fromWCharArray(L"面额%1元卡券成功购买%2张").arg(
                    QString::number(coupons[0].m_coupon.m_faceValue),
                    QString::number(coupons.size()));
            emit printLog(logContent);

            emit haveNewCoupon(coupons);
        }
    }
}
