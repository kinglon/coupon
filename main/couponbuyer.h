#ifndef COUPONBUYER_H
#define COUPONBUYER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QTimer>
#include "datamodel.h"
#include "mfhttpclient.h"
#include "settingmanager.h"

// 购买记录
class BuyRecord
{
public:
    // 购买记录，用于查询卡券信息
    QString m_buyRecordId;

    // 标志是否正在查询卡券
    bool m_queryCouponInfo = false;

    // 该购买记录的订单列表
    QVector<QString> m_orderIds;

    // 已购买的数量，包括已经购买还没有查到卡券
    int m_boughtCount = 0;

    // 标志是否允许查询卡券，有些错误后要暂停查询
    bool m_allowQueryCouponInfo = true;

public:
    // 判断是否需要查询卡券
    bool needQueryCouponInfo()
    {
        if (!m_queryCouponInfo && m_allowQueryCouponInfo && m_orderIds.size() < m_boughtCount)
        {
            return true;
        }

        return false;
    }
};

// 每种面额卡券购买状态
class CouponBuyStatus
{
public:
    // 购买卡券设置
    BuyCouponSetting m_buyCouponSetting;

    // 标志是否需要发起求购
    bool m_needSendWantBuyRequest = false;

    // 当前求购的record id
    QString m_recordId;

    // 取消求购次数，用于控制多次重试
    int m_cancelWantBuyTime = 0;

    // 标志是否正在取消中
    bool m_cancelling = false;

    // 库存数量
    int m_availCount = 0;

    // 购买的记录列表
    QVector<BuyRecord> m_buyRecords;

public:
    // 用于判断能否继续购卡
    bool canBuyCard()
    {
        // 没有库存，不能购买
        if (m_availCount <= 0)
        {
            return false;
        }

        // 没有求购，不能购买
        if (m_needSendWantBuyRequest)
        {
            return false;
        }

        return true;
    }

    // 获取能购买的卡券数量
    int getCanBuyCount()
    {
        for (const auto& buyRecord : m_buyRecords)
        {
            if (buyRecord.m_buyRecordId == m_recordId)
            {
                int buyCount = m_buyCouponSetting.m_willBuyCount - buyRecord.m_boughtCount;
                return buyCount;
            }
        }
        return 0;
    }

    // 增加购买数量
    void addBuyCount(const QString& recordId, int buyCount)
    {
        for (auto& buyRecord : m_buyRecords)
        {
            if (buyRecord.m_buyRecordId == recordId)
            {
                buyRecord.m_boughtCount += buyCount;
                buyRecord.m_allowQueryCouponInfo = true;
                break;
            }
        }
    }

    // 获取指定的购买记录
    BuyRecord* getBuyRecord(QString recordId)
    {
        for (auto& buyRecord : m_buyRecords)
        {
            if (buyRecord.m_buyRecordId == recordId)
            {
                return &buyRecord;
            }
        }

        return nullptr;
    }
};

// 卡券求购购买器
class CouponBuyer : public QObject
{
    Q_OBJECT
public:
    explicit CouponBuyer(QObject *parent = nullptr);

    void run();

    void requestStop() { m_requestStop = true; }

signals:
    // 成功购买到新卡券后发送信号
    void haveNewCoupon(QVector<GetCouponResult> result);

    void runFinish();

    void printLog(QString content);

private slots:
    void onMainTimer();

    // 成功买到卡券
    void onBuyCoupon(CouponBuyStatus* buyStatus, QString recordId, int count);

    // 查询卡券返回
    void onGetCouponCompletely(CouponBuyStatus* buyStatus, QString buyRecordId, bool success, QString errorMsg, QVector<GetCouponResult> result);

private:
    QTimer* m_mainTimer = nullptr;

    bool m_requestStop = false;

    QVector<CouponBuyStatus> m_couponBuyStatus;

    // 上一次发送求购请求的时间
    qint64 m_lastSendWantBuyReqTime = 0;

    // 上一次获取库存的时间
    qint64 m_lastGetFaceValStockTime = 0;

    // 总共要购买金额
    int m_totalWillBuyMoney = 0;

    // 当前已购买金额
    int m_totalBoughtMoney = 0;
};

#endif // COUPONBUYER_H
