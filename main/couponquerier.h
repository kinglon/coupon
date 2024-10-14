#ifndef COUPONQUERIER_H
#define COUPONQUERIER_H

#include <QObject>
#include <QVector>
#include "datamodel.h"

// 卡券查询器，批量查询卡券的信息
class CouponQuerier : public QObject
{
    Q_OBJECT
public:
    explicit CouponQuerier(QObject *parent = nullptr);

    void run(QString mobile, const QVector<Coupon>& coupons);

signals:
    void printLog(QString content);

    // 查完一张卡券后就通知
    void oneCouponFinish(Coupon coupon);

    // 查询完成
    void runFinish(bool success);

private:
    // 用于查询请求时的手机号
    QString m_mobile;

    // 待查询卡券列表
    QVector<Coupon> m_coupons;
};

#endif // COUPONQUERIER_H
