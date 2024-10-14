#ifndef CHARGEDIALOG_H
#define CHARGEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QCloseEvent>
#include "couponquerier.h"
#include "singlechargecontroller.h"

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
    void initCtrls();

    // 可以调用多次
    void initCouponTableView();

private slots:
    void onPrintLog(QString content);

    void onStartQueryButtonClicked();

    void onStartBindButtonClicked();

    void onImportCouponButtonClicked();

    void onDeleteCouponButtonClicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ChargeDialog *ui;

    // 卡券表格的模型
    QStandardItemModel m_couponModel;

    CouponQuerier* m_couponQuerier = nullptr;

    SingleChargeController* m_chargeController = nullptr;
};

#endif // CHARGEDIALOG_H
