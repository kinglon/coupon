﻿#ifndef COUPONBUYER_H
#define COUPONBUYER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QTimer>
#include "datamodel.h"
#include "mfhttpclient.h"
#include "settingmanager.h"

// 每种面额卡券购买状态
class CouponBuyStatus
{
public:
    // 购买卡券设置
    BuyCouponSetting m_buyCouponSetting;

    // 已购买的数量
    int m_boughtCount = 0;    

    // 标志是否需要发起求购
    bool m_needSendWantBuyRequest = false;

    // 求购的record id，用于取消求购
    QString m_recordId;

    // 取消求购次数，用于控制多次重试
    int m_cancelWantBuyTime = 0;

    // 标志是否正在取消中
    bool m_cancelling = false;

    // 库存数量
    int m_availCount = 0;

    // 购买的record_id，用于查询卡券信息
    QString m_buyRecordId;

    // 标志是否正在查询卡券
    bool m_queryCouponInfo = false;
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

private:
    QTimer* m_mainTimer = nullptr;

    bool m_requestStop = false;

    QVector<CouponBuyStatus> m_couponBuyStatus;

    // 上一次获取库存的时间
    qint64 m_lastGetFaceValStockTime = 0;
};

#endif // COUPONBUYER_H