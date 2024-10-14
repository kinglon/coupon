#ifndef YQBHTTPCLIENT_H
#define YQBHTTPCLIENT_H

#include "httpclientbase.h"
#include "datamodel.h"
#include <QJsonObject>

class ChargeResult
{
public:
    // 卡券
    Coupon m_coupon;

    // 充值结果
    bool m_success = false;

    // 壹钱包返回结果，充值失败时有效
    QString m_resultMsg;

    // 实际面值，充值成功时有效
    int m_realFaceValue = 0;

    // 查询时间
    QDateTime m_queryDateTime;
};

// 壹钱包充值
class YqbHttpClient : public HttpClientBase
{
    Q_OBJECT
public:
    explicit YqbHttpClient(QObject *parent = nullptr);

public:
    void charge(QString mobile, const Coupon& coupon, bool onlyQueryCoupon);

signals:
    // success=true 成功访问壹钱包，result充值结果
    // success=false 无法访问壹钱包，errorMsg原因
    void chargeCompletely(bool success, QString errorMsg, ChargeResult result);

protected:
    virtual void onHttpResponse(QNetworkReply *reply) override;

private:
    // 校验卡券
    void sendVerifyCouponRequest();

    // 获取校验卡券请求的body模板
    QJsonObject* getVerifyCouponBodyTemplate();

    // 获取充值请求的body模板
    QJsonObject* getRechargeBodyTemplate();

    void processVerifyCouponResponse(QNetworkReply *reply);

    void sendRechargeRequest();

    void processRechargeResponse(QNetworkReply *reply);

private:
    QString m_mobile;

    ChargeResult m_result;

    int m_retryCount = 0;

    // true 只是查询卡券信息，false是查询卡券信息并充值
    bool m_onlyQueryCoupon = false;

    // 校验卡券请求返回的data数据
    QJsonObject m_verifyCouponResponseData;
};

#endif // YQBHTTPCLIENT_H
