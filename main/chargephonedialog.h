#ifndef CHARGEPHONEDIALOG_H
#define CHARGEPHONEDIALOG_H

#include <QDialog>
#include "settingmanager.h"
#include "chargesettingmanager.h"

namespace Ui {
class ChargePhoneDialog;
}

// 充值手机号添加/编辑窗口
class ChargePhoneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChargePhoneDialog(QWidget *parent, ChargePhone chargePhone);
    ~ChargePhoneDialog();

    ChargePhone getChargePhone() { return m_chargePhone; }

private slots:
    void onOkButtonClicked();

private:
    void initCtrls();

private:
    Ui::ChargePhoneDialog *ui;

    ChargePhone m_chargePhone;
};

#endif // CHARGEPHONEDIALOG_H
