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

    // 上一次报告时间
    qint64 m_lastReportTime = 0;
};


class OrderStatusReporter : public QObject
{
    Q_OBJECT
protected:
    explicit OrderStatusReporter(QObject *parent = nullptr);

public:
    static OrderStatusReporter* getInstance();

    void reportOrderStatus(QString recordId, QString orderId, const ChargeResult& chargeResult);

    bool generateImage(QString fileName, QString content);

private slots:
    void onMainTimer();

private:
    QVector<ReportStatus> m_reportStatus;
};

#endif // ORDERSTATUSREPORTER_H
