#include "chargephonedialog.h"
#include "ui_chargephonedialog.h"
#include "uiutil.h"
#include <QUuid>

ChargePhoneDialog::ChargePhoneDialog(QWidget *parent, ChargePhone chargePhone) :
    QDialog(parent),
    ui(new Ui::ChargePhoneDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    m_chargePhone = chargePhone;
    initCtrls();
}

ChargePhoneDialog::~ChargePhoneDialog()
{
    delete ui;
}



void ChargePhoneDialog::initCtrls()
{
    ui->phoneNumberEdit->setText(m_chargePhone.m_phoneNumber);
    ui->moneyCountEdit->setText(QString::number(m_chargePhone.m_moneyCount));
    ui->chargedMoneyEdit->setText(QString::number(m_chargePhone.m_chargeMoney));
    ui->priorityEdit->setText(QString::number(m_chargePhone.m_priority));
    ui->remarkEdit->setText(m_chargePhone.m_remark);

    connect(ui->okButton, &QPushButton::clicked, [this]() {
        onOkButtonClicked();
    });

    connect(ui->cancelButton, &QPushButton::clicked, [this]() {
        close();
    });
}

void ChargePhoneDialog::onOkButtonClicked()
{
    if (ui->phoneNumberEdit->text().isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"手机号不能为空"));
        return;
    }

    int moneyCount = ui->moneyCountEdit->text().toInt();
    if (moneyCount <= 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"金额填写有误"));
        return;
    }

    bool ok = false;
    int chargedMoneyCount = ui->chargedMoneyEdit->text().toInt(&ok);
    if (!ok || chargedMoneyCount < 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"已充金额填写有误"));
        return;
    }

    int priority = ui->priorityEdit->text().toInt();
    if (priority <= 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"优先级填写有误"));
        return;
    }

    m_chargePhone.m_phoneNumber = ui->phoneNumberEdit->text();
    m_chargePhone.m_moneyCount = moneyCount;
    m_chargePhone.m_chargeMoney = chargedMoneyCount;
    m_chargePhone.m_priority = priority;
    m_chargePhone.m_remark = ui->remarkEdit->text();
    if (m_chargePhone.m_id.isEmpty())
    {
        m_chargePhone.m_id = QUuid::createUuid().toString();
    }
    done(Accepted);
}
