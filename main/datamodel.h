#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>

// 卡券
class Coupon
{
public:
    // 卡号
    QString m_couponId;

    // 卡密
    QString m_couponPassword;

    // 面值
    int m_faceValue = 0;

    bool isValid()
    {
        if (m_couponId.isEmpty() || m_couponPassword.isEmpty() || m_faceValue <= 0)
        {
            return false;
        }
        return true;
    }
};

#endif // DATAMODEL_H
