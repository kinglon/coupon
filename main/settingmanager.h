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

class BuyCouponSetting
{
public:
    // 面额
    int m_faceVal = 0;

    // 要购买的数量
    int m_willBuyCount = 0;

    // 折扣
    int m_discount = 950;
};

class YqbSetting
{
public:
    // 壹钱包token
    QString m_yqbToken;

    // 刷新使用的手机号
    QString m_mobileNumber;

    // 刷新使用的卡券号码
    QString m_couponId;

    // 刷新使用的卡券密码
    QString m_couponPassword;

    bool isValid()
    {
        if (!m_yqbToken.isEmpty() && !m_mobileNumber.isEmpty() && !m_couponId.isEmpty() && !m_couponPassword.isEmpty())
        {
            return true;
        }
        return false;
    }
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

    // 壹钱包设置
    YqbSetting m_yqbSetting;

    // 求购设置
    QVector<BuyCouponSetting> m_buyCouponSetting;

    // 充值手机列表
    QVector<ChargePhone> m_chargePhones;

    // 导入的卡券列表
    QVector<Coupon> m_coupons;

    // 查询卡券库存间隔，单位毫秒
    int m_mfQueryStockInterval = 5000;
};
