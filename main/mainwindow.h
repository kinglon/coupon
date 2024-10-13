#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QStandardItemModel>
#include "settingmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCtrlDShortcut();

    void onStartBuyButtonClicked();

    void onCancelBuyButtonClicked();

    void onAddPhoneButtonClicked();

    void onEditPhoneTableView(int row);

    void onDeletePhoneTableView(int row);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void initCtrls();

    // 可以调用多次
    void initPhoneTableView();

private:
    Ui::MainWindow *ui;

    QShortcut* m_ctrlDShortcut = nullptr;

    // 充值手机表格的模型
    QStandardItemModel m_phoneModel;
};
#endif // MAINWINDOW_H
