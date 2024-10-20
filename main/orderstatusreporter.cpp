#include "orderstatusreporter.h"
#include <Windows.h>
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QFile>
#include "settingmanager.h"

#define MAX_SEND_REQUEST_COUNT 3

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
        else if (chargeResult.m_resultMsg.indexOf(QString::fromWCharArray(L"流水号错误")) >= 0)
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

    if (!reportStatus.m_orderStatus.m_success)
    {
        QString imageFileName = orderId + ".jpg";
        if (!generateImage(imageFileName, chargeResult))
        {
            return;
        }
        reportStatus.m_orderStatus.m_imageUrl = SettingManager::getInstance()->m_mfSetting.m_callbackHost + "/" + imageFileName;
    }
    m_reportStatus.append(reportStatus);
}

bool OrderStatusReporter::generateImage(QString fileName, const ChargeResult& chargeResult)
{
    QImage image(1000, 500, QImage::Format_RGB32);
    image.fill(Qt::white); // Fill the image with white color

    // Create a QPainter object to draw on the image
    QPainter painter(&image);

    // Set the font and color for the text
    painter.setFont(QFont("Arial", 20));
    painter.setPen(Qt::black);

    // Draw the text on the image
    QVector<QString> orderInfos;
    orderInfos.append(QString::fromWCharArray(L"卡号：%1").arg(chargeResult.m_coupon.m_couponId));
    orderInfos.append(QString::fromWCharArray(L"卡密：%1").arg(chargeResult.m_coupon.m_couponPassword));
    orderInfos.append(QString::fromWCharArray(L"查询时间: %1").arg(chargeResult.m_queryDateTime.toString("yyyy/MM/dd hh:mm:ss")));
    orderInfos.append(QString::fromWCharArray(L"购买面值：%1").arg(QString::number(chargeResult.m_coupon.m_faceValue)));
    orderInfos.append(QString::fromWCharArray(L"实际面值：%1").arg(QString::number(chargeResult.m_realFaceValue)));
    orderInfos.append(QString::fromWCharArray(L"壹钱包返回结果：%1").arg(chargeResult.m_resultMsg));
    for (int i=0; i<orderInfos.size(); i++)
    {
        painter.drawText(10, 50+40*i, orderInfos[i]);
    }

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
        if (!reportStatus.m_sendingRequest)
        {
            QString orderId = reportStatus.m_orderStatus.m_orderId;
            MfHttpClient* mfClient = new MfHttpClient(this);
            connect(mfClient, &MfHttpClient::reportOrderStatusCompletely, [this, orderId, mfClient](bool success, QString errorMsg) {
                if (!success)
                {                    
                    if (errorMsg.indexOf(QString::fromWCharArray(L"已处理过")) >= 0)
                    {
                        success = true;
                    }
                }

                for (auto it=m_reportStatus.begin(); it!=m_reportStatus.end(); it++)
                {
                    if (it->m_orderStatus.m_orderId == orderId)
                    {
                        it->m_sendingRequest = false;
                        if (success)
                        {
                            qInfo("successful to report status of order: %s", orderId.toStdString().c_str());
                            m_reportStatus.erase(it);
                        }
                        else
                        {
                            QString logContent = QString::fromWCharArray(L"上报订单(%1)状态失败：%2").arg(
                                        orderId.toStdString().c_str(),
                                        errorMsg.toStdString().c_str());
                            emit printLog(logContent);
                            if (it->m_sendReqCount >= MAX_SEND_REQUEST_COUNT)
                            {
                                m_reportStatus.erase(it);
                            }
                        }
                        break;
                    }
                }

                mfClient->deleteLater();
            });
            mfClient->reportOrderStatus(reportStatus.m_orderStatus);
            qInfo("report status of order: %s", reportStatus.m_orderStatus.m_orderId.toStdString().c_str());
            reportStatus.m_sendReqCount++;
            reportStatus.m_sendingRequest = true;
        }
    }
}
