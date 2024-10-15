#ifndef SINGLECHARGECONTROLLER_H
#define SINGLECHARGECONTROLLER_H

#include <QObject>
#include "datamodel.h"

// 单手机多张卡券充值
class SingleChargeController : public QObject
{
    Q_OBJECT
public:
    explicit SingleChargeController(QObject *parent = nullptr);

    void run(QString mobile, int chargeMoney, const QVector<Coupon>& coupons);

signals:
    void printLog(QString content);

    void couponStatusChange(QString couponPassword, QString status);

    void runFinish(bool success);

private:
    void doCharge();

private:
    QString m_mobile;

    int m_chargeMoney = 0;

    QVector<Coupon> m_coupons;

    // 当前正在充值的卡券索引
    int m_currentChargeCouponIndex = 0;

    // 累计已充的金额
    int m_sumChargeMoney = 0;
};

#endif // SINGLECHARGECONTROLLER_H
