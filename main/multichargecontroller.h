#ifndef MULTICHARGECONTROLLER_H
#define MULTICHARGECONTROLLER_H

#include <QObject>
#include "couponbuyer.h"
#include "singlechargecontroller.h"
#include "orderstatusreporter.h"

// 自动求购充值
class MultiChargeController : public QObject
{
    Q_OBJECT
public:
    explicit MultiChargeController(QObject *parent = nullptr);

    void run();

    void requestStop();

signals:
    // 充值金额发生变化时发送
    void chargeChange(QString mobile);

    void runFinish(bool success);

    void printLog(QString content);

private:
    void doCharge();

    void doCharge(QString mobile, int chargeMoney, const QVector<Coupon>& coupons);

    bool isNeedCharge();

private:
    CouponBuyer* m_couponBuyer = nullptr;

    SingleChargeController* m_chargeController = nullptr;

    // 购买卡券列表
    QVector<GetCouponResult> m_coupons;
};

#endif // MULTICHARGECONTROLLER_H
