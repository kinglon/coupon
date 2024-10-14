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

    void runFinish(bool success);
};

#endif // SINGLECHARGECONTROLLER_H
