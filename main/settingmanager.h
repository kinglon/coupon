#pragma once

#include <QString>
#include <QVector>
#include "datamodel.h"

// 充值手机
class ChargePhone
{
public:
    // id
    QString m_id;

    // 手机号
    QString m_phoneNumber;

    // 要充的总金额
    int m_moneyCount = 0;

    // 已充金额
    int m_chargeMoney = 0;

    // 优先级
    int m_priority = 1;

    // 备注
    QString m_remark;
};

class SettingManager
{
protected:
    SettingManager();

public:
    static SettingManager* getInstance();

    void save();

    void updateChargePhone(const ChargePhone& chargePhone);

    void deleteChargePhone(QString id);

    // 获取所有手机充值金额总和
    int getTotalChargeMoney();

private:
    void load();

public:
    int m_nLogLevel = 2;  // info

    // 壹钱包token
    QString m_yqbToken;

    // 每个面额求购的数量，对应50-1000面额
    QVector<int> m_wantBuyCounts;

    // 充值手机列表
    QVector<ChargePhone> m_chargePhones;

    // 导入的卡券列表
    QVector<Coupon> m_coupons;
};
