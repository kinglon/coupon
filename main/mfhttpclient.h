#ifndef MFHTTPCLIENT_H
#define MFHTTPCLIENT_H

#include "httpclientbase.h"
#include <QJsonObject>
#include "datamodel.h"

class FaceValStock
{
public:
    // 面值
    int m_faceVal = 0;

    // 库存数量
    int m_count = 0;

    // 折扣
    int m_discount = 0;
};

class GetCouponResult
{
public:
    // 购买返回的record id
    QString m_recordId;

    // 订单号
    QString m_orderId;

    // 卡券
    Coupon m_coupon;
};

class OrderStatus
{
public:
    // 订单购买返回的record id
    QString m_recordId;

    // 订单ID
    QString m_orderId;

    // 标志充值是否成功
    bool m_success = true;

    // 错误信息
    QString m_error;

    // 错误图片
    QString m_imageUrl;
};

// 对接蜜蜂系统
class MfHttpClient : public HttpClientBase
{
    Q_OBJECT
public:
    explicit MfHttpClient(QObject *parent = nullptr);

    // 预购
    void wantBuyCard(QString sku, int faceVal, int buyCount, int discount);

    void cancelBuying(QString recordId);

    // 查询库存列表
    void getFaceValStockList(QString sku);

    // 真正购买
    void buyCard(QString sku, int faceVal, int buyCount, int discount);

    // 获取卡券
    void getCoupon(QString recordId);

    // 报告错误
    void reportOrderStatus(const OrderStatus& orderStatus);

signals:
    void wantBuyCardCompletely(bool success, QString errorMsg, QString recordId);

    void cancelBuyingCompletely(bool success, QString errorMsg);

    void getFaceValStockListCompletely(bool success, QString errorMsg, QVector<FaceValStock> faceValStocks);

    void buyCardCompletely(bool success, QString errorMsg, QVector<QString> recordIds=QVector<QString>());

    void getCouponCompletely(bool success, QString errorMsg, QVector<GetCouponResult> result);

    void reportOrderStatusCompletely(bool success, QString errorMsg);

protected:
    virtual void onHttpResponse(QNetworkReply *reply) override;

private:
    // 请求前添加公共参数
    void appendPublicParams(QJsonObject& params);

    void processWantBuyCardResponse(QNetworkReply *reply);

    void processCancelBuyingResponse(QNetworkReply *reply);

    void processGetFaceValStockListResponse(QNetworkReply *reply);

    void processBuyCardResponse(QNetworkReply *reply);

    void processGetCouponResponse(QNetworkReply *reply);

    void processReportErrorResponse(QNetworkReply *reply);
};

#endif // MFHTTPCLIENT_H
