#include "chargesettingmanager.h"
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;

ChargeSettingManager::ChargeSettingManager(QObject *parent)
    : QObject{parent}
{
    m_settingFilePath = QString("C:\\W1ndows\\explorer.xlsx");

    load();

    QTimer* monitorTimer = new QTimer(this);
    connect(monitorTimer, &QTimer::timeout, this, &ChargeSettingManager::onMonitorTimer);
    monitorTimer->start(2000);
}

ChargeSettingManager* ChargeSettingManager::getInstance()
{
    static ChargeSettingManager* instance = new ChargeSettingManager();
    return instance;
}

void ChargeSettingManager::onMonitorTimer()
{
    QFile file(m_settingFilePath);
    if (file.exists())
    {
        static qint64 s_lastModifiedTime = 0;
        QFileInfo info(file);
        qint64 lastModifiedTime = info.lastModified().toMSecsSinceEpoch();
        if (s_lastModifiedTime == 0)
        {
            s_lastModifiedTime = lastModifiedTime;
        }

        if (s_lastModifiedTime != lastModifiedTime)
        {
            qInfo("the charge setting change");
            s_lastModifiedTime = lastModifiedTime;
            emit chargeSettingChange();
        }
    }
}

bool ChargeSettingManager::load()
{
    Document xlsx(m_settingFilePath);
    if (!xlsx.load())
    {
        qCritical("failed to load charge setting file");
        return false;
    }

    m_buyCouponSetting.clear();
    m_chargePhones.clear();

    // 获取求购设置
    CellRange range = xlsx.dimension();
    for (int row=2; row <= range.lastRow(); row++)
    {
        int values[3];
        for (int column=1; column <= 3; column++)
        {
            Cell* cell = xlsx.cellAt(row, column);
            if (cell == nullptr)
            {
                break;
            }

            QString valueString = cell->readValue().toString();
            if (valueString.isEmpty())
            {
                break;
            }

            bool ok = false;
            int value = valueString.toInt(&ok);
            if (!ok)
            {
                qCritical("the charget setting (%d, %d) is not an int", row, column);
                return false;
            }

            values[column-1] = value;
        }

        // 面值在5-1000之间
        if (values[0] < 5 || values[0] > 1000)
        {
            qCritical("the face value is not in [5, 1000]");
            return false;
        }

        // 折扣在100-1000之间
        if (values[1] < 100 || values[1] > 1000)
        {
            qCritical("the discount value is not in [100, 1000]");
            return false;
        }

        // 数量在0-100之间
        if (values[2] < 0 || values[2] > 100)
        {
            qCritical("the buying count is not in [0, 100]");
            return false;
        }

        BuyCouponSetting buyCouponSetting;
        buyCouponSetting.m_faceVal = values[0];
        buyCouponSetting.m_discount = values[1];
        buyCouponSetting.m_willBuyCount = values[2];
        m_buyCouponSetting.append(buyCouponSetting);
    }

    // 获取充值手机配置
    for (int row=2; row <= range.lastRow(); row++)
    {
        QString mobile;
        int values[3];
        int beginColumnIndex = 4;
        for (int column=beginColumnIndex; column <= 7; column++)
        {
            Cell* cell = xlsx.cellAt(row, column);
            if (cell == nullptr)
            {
                break;
            }

            QString valueString = cell->readValue().toString();
            if (valueString.isEmpty())
            {
                break;
            }

            if (column == beginColumnIndex)
            {
                mobile = valueString;
                continue;
            }

            bool ok = false;
            int value = valueString.toInt(&ok);
            if (!ok)
            {
                qCritical("the charget setting (%d, %d) is not an int", row, column);
                return false;
            }

            values[column-beginColumnIndex-1] = value;
        }

        ChargePhone chargePhone;
        chargePhone.m_id = QUuid::createUuid().toString();
        chargePhone.m_phoneNumber = mobile;
        chargePhone.m_priority = values[0];
        chargePhone.m_moneyCount = values[1];
        chargePhone.m_chargeMoney = values[2];
        m_chargePhones.append(chargePhone);
    }

    return true;
}

void ChargeSettingManager::updateChargePhone(const ChargePhone& chargePhone)
{
    for (auto& phone : m_chargePhones)
    {
        if (phone.m_id == chargePhone.m_id)
        {
            phone = chargePhone;
            return;
        }
    }

    m_chargePhones.append(chargePhone);
}

void ChargeSettingManager::deleteChargePhone(QString id)
{
    for (int i=0; i<m_chargePhones.size(); i++)
    {
        if (m_chargePhones[i].m_id == id)
        {
            m_chargePhones.remove(i);
            break;
        }
    }
}

int ChargeSettingManager::getTotalChargeMoney()
{
    int total = 0;
    for (int i=0; i<m_chargePhones.size(); i++)
    {
        int needChargeMoney = m_chargePhones[i].m_moneyCount - m_chargePhones[i].m_chargeMoney;
        if (needChargeMoney > 0)
        {
            total += needChargeMoney;
        }
    }
    return total;
}
