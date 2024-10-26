﻿#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = nullptr);
    ~SettingDialog();

private:
    void initCtrls();

private slots:
    void onOkButtonClicked();

private:
    Ui::SettingDialog *ui;
};

#endif // SETTINGDIALOG_H
