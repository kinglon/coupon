#ifndef CHARGEDIALOG_H
#define CHARGEDIALOG_H

#include <QDialog>

namespace Ui {
class ChargeDialog;
}

// 单个手机充值窗口
class ChargeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChargeDialog(QWidget *parent=nullptr);
    ~ChargeDialog();

private:
    Ui::ChargeDialog *ui;
};

#endif // CHARGEDIALOG_H
