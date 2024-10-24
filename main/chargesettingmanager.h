#ifndef CHARGESETTINGMANAGER_H
#define CHARGESETTINGMANAGER_H

#include <QObject>

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

// 求购充值手机配置管理器
class ChargeSettingManager : public QObject
{
    Q_OBJECT
public:
    explicit ChargeSettingManager(QObject *parent = nullptr);

    static ChargeSettingManager* getInstance();

    bool load();

    // 导出充值结果
    bool exportChargeResult();

    void updateChargePhone(const ChargePhone& chargePhone);

    void deleteChargePhone(QString id);

    // 获取所有手机充值金额总和
    int getTotalChargeMoney();

signals:
    // 配置表格变化重新加载后触发
    void chargeSettingChange();

private slots:
    void onMonitorTimer();

public:
    QString m_settingFilePath;

    // 求购设置
    QVector<BuyCouponSetting> m_buyCouponSetting;

    // 充值手机列表
    QVector<ChargePhone> m_chargePhones;
};

#endif // CHARGESETTINGMANAGER_H
