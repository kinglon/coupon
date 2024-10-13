#include "settingmanager.h"
#include <QFile>
#include "Utility/ImPath.h"
#include "Utility/ImCharset.h"
#include "Utility/LogMacro.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SettingManager::SettingManager()
{
    load();
}

SettingManager* SettingManager::getInstance()
{
    static SettingManager* pInstance = new SettingManager();
	return pInstance;
}

void SettingManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs.json";    
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(L"failed to open the basic configure file : %s", strConfFilePath.c_str());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();
    m_nLogLevel = root["log_level"].toInt();
}

void SettingManager::save()
{
    // todo by yejinlong
}

void SettingManager::updateChargePhone(const ChargePhone& chargePhone)
{
    for (auto& phone : m_chargePhones)
    {
        if (phone.m_id == chargePhone.m_id)
        {
            phone = chargePhone;
            save();
            return;
        }
    }

    m_chargePhones.append(chargePhone);
    save();
}

void SettingManager::deleteChargePhone(QString id)
{
    for (int i=0; i<m_chargePhones.size(); i++)
    {
        if (m_chargePhones[i].m_id == id)
        {
            m_chargePhones.remove(i);
            break;
        }
    }
    save();
}
