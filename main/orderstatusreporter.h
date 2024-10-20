#ifndef ORDERSTATUSREPORTER_H
#define ORDERSTATUSREPORTER_H

#include <QObject>
#include <QVector>
#include "mfhttpclient.h"
#include "yqbhttpclient.h"

class ReportStatus
{
public:
    // 订单状态
    OrderStatus m_orderStatus;

    // 发送请求的次数
    int m_sendReqCount = 0;

    // 标志是否正在发送请求
    bool m_sendingRequest = false;
};


class OrderStatusReporter : public QObject
{
    Q_OBJECT
protected:
    explicit OrderStatusReporter(QObject *parent = nullptr);

public:
    static OrderStatusReporter* getInstance();

    void reportOrderStatus(QString recordId, QString orderId, const ChargeResult& chargeResult);

    bool generateImage(QString fileName, const ChargeResult& chargeResult);

signals:
    void printLog(QString content);

private slots:
    void onMainTimer();

private:
    QVector<ReportStatus> m_reportStatus;
};

#endif // ORDERSTATUSREPORTER_H
