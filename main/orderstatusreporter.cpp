#include "orderstatusreporter.h"
#include <Windows.h>
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QFile>

#define REPORT_INTERVAL 3000

OrderStatusReporter::OrderStatusReporter(QObject *parent)
    : QObject{parent}
{
    QTimer* mainTimer = new QTimer(this);
    connect(mainTimer, &QTimer::timeout, this, &OrderStatusReporter::onMainTimer);
    mainTimer->start(500);
}

OrderStatusReporter* OrderStatusReporter::getInstance()
{
    static OrderStatusReporter* instance = new OrderStatusReporter();
    return instance;
}

void OrderStatusReporter::reportOrderStatus(QString recordId, QString orderId, const ChargeResult& chargeResult)
{
    ReportStatus reportStatus;
    reportStatus.m_orderStatus.m_recordId = recordId;
    reportStatus.m_orderStatus.m_orderId = orderId;
    reportStatus.m_orderStatus.m_success = chargeResult.m_success;
    if (chargeResult.m_success)
    {
        if (chargeResult.m_coupon.m_faceValue != chargeResult.m_realFaceValue)
        {
            reportStatus.m_orderStatus.m_success = false;
            reportStatus.m_orderStatus.m_error = QString::fromWCharArray(L"面值不符（我绑卡了，按实际面值结算）");
        }
    }
    else
    {
        if (chargeResult.m_resultMsg.indexOf(QString::fromWCharArray(L"已被使用")) >= 0)
        {
            reportStatus.m_orderStatus.m_error = QString::fromWCharArray(L"卡已被使用/失效");
        }
        else if (chargeResult.m_resultMsg.indexOf(QString::fromWCharArray(L"卡密错误")) >= 0 ||
                 chargeResult.m_resultMsg.indexOf(QString::fromWCharArray(L"卡号错误")) >= 0)
        {
            reportStatus.m_orderStatus.m_error = QString::fromWCharArray(L"卡号/卡密错误");
        }
        else if (chargeResult.m_resultMsg.indexOf(QString::fromWCharArray(L"未激活")) >= 0)
        {
            reportStatus.m_orderStatus.m_error = QString::fromWCharArray(L"未激活");
        }
        else
        {
            reportStatus.m_orderStatus.m_error = QString::fromWCharArray(L"其它原因");
        }
    }

    QString orderInfo = QString::fromWCharArray(L"卡号：%1，卡密：%2，查询时间：%3，购买面值：%4，实际面值：%5，壹钱包返回结果：%6")
            .arg(chargeResult.m_coupon.m_couponId, chargeResult.m_coupon.m_couponPassword,
                 chargeResult.m_queryDateTime.toString("yyyy/MM/dd hh:mm"),
                 QString::number(chargeResult.m_coupon.m_faceValue),
                 QString::number(chargeResult.m_realFaceValue),
                 chargeResult.m_resultMsg);
    QString imageFileName = orderId + ".jpg";
    if (!generateImage(imageFileName, orderInfo))
    {
        return;
    }

    reportStatus.m_orderStatus.m_imageUrl = "http://beekeep.mkwen.cn/" + imageFileName;
    m_reportStatus.append(reportStatus);
}

bool OrderStatusReporter::generateImage(QString fileName, QString content)
{
    QImage image(1000, 300, QImage::Format_RGB32);
    image.fill(Qt::white); // Fill the image with white color

    // Create a QPainter object to draw on the image
    QPainter painter(&image);

    // Set the font and color for the text
    painter.setFont(QFont("Arial", 20));
    painter.setPen(Qt::black);

    // Draw the text on the image
    painter.drawText(QPoint(5, 20), content);

    // Save the image to a file
    QString savedPath = "C:\\issue\\";
    CreateDirectory(savedPath.toStdWString().c_str(), nullptr);
    if (!image.save(savedPath + fileName))
    {
        qCritical("failed to save the image of %s", fileName.toStdString().c_str());
        return false;
    }

    return true;
}

void OrderStatusReporter::onMainTimer()
{
    for (auto& reportStatus : m_reportStatus)
    {
        if (GetTickCount64() - reportStatus.m_lastReportTime >= REPORT_INTERVAL)
        {
            QString orderId = reportStatus.m_orderStatus.m_orderId;
            MfHttpClient* mfClient = new MfHttpClient(this);
            connect(mfClient, &MfHttpClient::reportOrderStatusCompletely, [this, orderId, mfClient](bool success, QString errorMsg) {
                if (!success)
                {
                    qCritical(errorMsg.toStdString().c_str());
                }
                else
                {
                    for (auto it=m_reportStatus.begin(); it!=m_reportStatus.end(); it++)
                    {
                        if (it->m_orderStatus.m_orderId == orderId)
                        {
                            m_reportStatus.erase(it);
                            break;
                        }
                    }
                }
                mfClient->deleteLater();
            });
            mfClient->reportOrderStatus(reportStatus.m_orderStatus);
            qInfo("report status of order: %s", reportStatus.m_orderStatus.m_orderId.toStdString().c_str());
            reportStatus.m_lastReportTime = GetTickCount64();
        }
    }
}
