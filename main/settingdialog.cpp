#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "settingmanager.h"
#include "uiutil.h"

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::initCtrls()
{
    const auto& yqbSetting = SettingManager::getInstance()->m_yqbSetting;
    ui->yqbTokenEdit->setText(yqbSetting.m_yqbToken);
    ui->yqbMobileEdit->setText(yqbSetting.m_mobileNumber);
    ui->yqbCouponIdEdit->setText(yqbSetting.m_couponId);
    ui->yqbCouponPasswordEdit->setText(yqbSetting.m_couponPassword);

    ui->queryStockIntervalEdit->setText(QString::number(SettingManager::getInstance()->m_mfQueryStockInterval/1000));

    const auto& mfSetting = SettingManager::getInstance()->m_mfSetting;
    ui->mfAppKeyEdit->setText(mfSetting.m_appKey);
    ui->mfSecretEdit->setText(mfSetting.m_appSecret);
    ui->mfCallbackHostEdit->setText(mfSetting.m_callbackHost);

    connect(ui->okButton, &QPushButton::clicked, [this]() {
        onOkButtonClicked();
    });

    connect(ui->cancelButton, &QPushButton::clicked, [this]() {
        close();
    });
}

void SettingDialog::onOkButtonClicked()
{
    YqbSetting yqbSetting;
    yqbSetting.m_yqbToken = ui->yqbTokenEdit->text();
    yqbSetting.m_mobileNumber = ui->yqbMobileEdit->text();
    yqbSetting.m_couponId = ui->yqbCouponIdEdit->text();
    yqbSetting.m_couponPassword = ui->yqbCouponPasswordEdit->text();
    if (!yqbSetting.isValid())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请正确填写壹钱包设置"));
        return;
    }

    bool ok = false;
    int queryStockInterval = ui->queryStockIntervalEdit->text().toInt(&ok);
    if (!ok || queryStockInterval <= 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"请正确填写查询库存间隔秒数设置"));
        return;
    }

    MfSetting mfSetting;
    mfSetting.m_appKey = ui->mfAppKeyEdit->text();
    mfSetting.m_appSecret = ui->mfSecretEdit->text();
    mfSetting.m_callbackHost = ui->mfCallbackHostEdit->text();
    if (!mfSetting.isValid())
    {
        UiUtil::showTip(QString::fromWCharArray(L"请正确填写蜜蜂设置"));
        return;
    }

    SettingManager::getInstance()->m_yqbSetting = yqbSetting;
    SettingManager::getInstance()->m_mfSetting = mfSetting;
    SettingManager::getInstance()->m_mfQueryStockInterval = queryStockInterval * 1000;
    SettingManager::getInstance()->save();

    done(Accepted);
}
