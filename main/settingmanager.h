#pragma once

#include <QString>
#include <QVector>
#include "datamodel.h"

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

// 蜜蜂系统设置
class MfSetting
{
public:
    // APP KEY
    QString m_appKey = "756384562";

    // APP SECRET
    QString m_appSecret = "66fa92d96da87";

    // 回调host
    QString m_callbackHost = "http://beekeep.mkwen.cn";

    bool isValid()
    {
        if (m_appKey.isEmpty() || m_appSecret.isEmpty() || m_callbackHost.isEmpty())
        {
            return false;
        }

        return true;
    }
};

class SettingManager
{
protected:
    SettingManager();

public:
    static SettingManager* getInstance();

    void save();    

private:
    void load();

public:
    int m_nLogLevel = 2;  // info

    // 壹钱包设置
    YqbSetting m_yqbSetting;    

    // 导入的卡券列表
    QVector<Coupon> m_coupons;

    // 查询卡券库存间隔，单位毫秒
    int m_mfQueryStockInterval = 5000;

    // 蜜蜂设置
    MfSetting m_mfSetting;

    // 导入导出目录，存放充值记录表、手机充值表
    QString m_externalPath = "C:\\W1ndows";
};
