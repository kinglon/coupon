#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingdialog.h"
#include "chargephonedialog.h"
#include "chargedialog.h"
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();

    m_ctrlDShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(m_ctrlDShortcut, &QShortcut::activated, this, &MainWindow::onCtrlDShortcut);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCtrls()
{
    // 初始化求购面额数量
    QLineEdit* edits[] = {ui->fv5CountEdit, ui->fv10CountEdit, ui->fv50CountEdit, ui->fv100CountEdit,
                         ui->fv200CountEdit, ui->fv500CountEdit, ui->fv1000CountEdit};
    QVector<int> wantBuyCounts = SettingManager::getInstance()->m_wantBuyCounts;
    for (int i=0; i<sizeof(edits)/sizeof(edits[0]) && i<wantBuyCounts.size(); i++)
    {
        edits[i]->setText(QString::number(wantBuyCounts[i]));
    }

    // 初始化充值手机列表
    initPhoneTableView();

    connect(ui->startBuyButton, &QPushButton::clicked, [this]() {
        onStartBuyButtonClicked();
    });

    connect(ui->cancelBuyButton, &QPushButton::clicked, [this]() {
        onCancelBuyButtonClicked();
    });

    connect(ui->settingButton, &QPushButton::clicked, [this]() {
        SettingDialog dlg(this);
        dlg.show();
        dlg.exec();
    });

    connect(ui->addPhoneButton, &QPushButton::clicked, [this]() {
        onAddPhoneButtonClicked();
    });

    connect(ui->importChargeButton, &QPushButton::clicked, []() {
        ChargeDialog dlg;
        dlg.show();
    });
}

void MainWindow::initPhoneTableView()
{
    ui->phoneTableView->installEventFilter(this);
    ui->phoneTableView->setModel(&m_phoneModel);

    m_phoneModel.clear();
    m_phoneModel.setColumnCount(5);
    m_phoneModel.setHeaderData(0, Qt::Horizontal, QString::fromWCharArray(L"手机号"));
    m_phoneModel.setHeaderData(1, Qt::Horizontal, QString::fromWCharArray(L"优先级"));
    m_phoneModel.setHeaderData(2, Qt::Horizontal, QString::fromWCharArray(L"金额"));
    m_phoneModel.setHeaderData(3, Qt::Horizontal, QString::fromWCharArray(L"已充金额"));
    m_phoneModel.setHeaderData(4, Qt::Horizontal, QString::fromWCharArray(L"备注"));

    QVector<ChargePhone>& phones = SettingManager::getInstance()->m_chargePhones;
    for (int i=0; i<phones.size(); i++)
    {
        QStandardItem* item = new QStandardItem(phones[i].m_phoneNumber);
        item->setTextAlignment(Qt::AlignCenter);
        m_phoneModel.setItem(i, 0, item);

        item = new QStandardItem(QString::number(phones[i].m_priority));
        item->setTextAlignment(Qt::AlignCenter);
        m_phoneModel.setItem(i, 1, item);

        item = new QStandardItem(QString::number(phones[i].m_moneyCount));
        item->setTextAlignment(Qt::AlignCenter);
        m_phoneModel.setItem(i, 2, item);

        item = new QStandardItem(QString::number(phones[i].m_chargeMoney));
        item->setTextAlignment(Qt::AlignCenter);
        m_phoneModel.setItem(i, 3, item);

        item = new QStandardItem(phones[i].m_remark);
        m_phoneModel.setItem(i, 4, item);

        m_phoneModel.setData(m_phoneModel.index(i,0), phones[i].m_id, Qt::UserRole);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->phoneTableView && event->type() == QEvent::ContextMenu)
    {
        QModelIndex index = ui->phoneTableView->currentIndex();
        if (index.isValid())
        {
            QMenu menu;
            menu.addAction(QString::fromWCharArray(L"编辑"));
            menu.addAction(QString::fromWCharArray(L"删除"));
            QAction * action = menu.exec(cursor().pos());
            if (action && action->text() == QString::fromWCharArray(L"编辑"))
            {
                onEditPhoneTableView(index.row());
            }
            else if (action && action->text() == QString::fromWCharArray(L"删除"))
            {
                onDeletePhoneTableView(index.row());
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onCtrlDShortcut()
{

}

void MainWindow::onStartBuyButtonClicked()
{
    // todo by yejinlong
}

void MainWindow::onCancelBuyButtonClicked()
{
    // todo by yejinlong
}

void MainWindow::onAddPhoneButtonClicked()
{
    ChargePhoneDialog dlg(this, ChargePhone());
    dlg.show();
    if (dlg.exec() != dlg.Accepted)
    {
        return;
    }

    SettingManager::getInstance()->updateChargePhone(dlg.getChargePhone());
    initPhoneTableView();
}

void MainWindow::onEditPhoneTableView(int row)
{
    QString id = m_phoneModel.data(m_phoneModel.index(row, 0), Qt::UserRole).toString();
    ChargePhone chargePhone;
    for (auto& phone : SettingManager::getInstance()->m_chargePhones)
    {
        if (phone.m_id == id)
        {
            chargePhone = phone;
            break;
        }
    }

    if (chargePhone.m_id.isEmpty())
    {
        return;
    }

    ChargePhoneDialog dlg(this, chargePhone);
    dlg.show();
    if (dlg.exec() != dlg.Accepted)
    {
        return;
    }

    SettingManager::getInstance()->updateChargePhone(dlg.getChargePhone());
    initPhoneTableView();
}

void MainWindow::onDeletePhoneTableView(int row)
{
    QString id = m_phoneModel.data(m_phoneModel.index(row, 0), Qt::UserRole).toString();
    SettingManager::getInstance()->deleteChargePhone(id);
    initPhoneTableView();
}
