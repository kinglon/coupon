#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingdialog.h"
#include "chargephonedialog.h"
#include "chargedialog.h"
#include "uiutil.h"
#include <QMenu>
#include <QDateTime>

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

    connect(&m_loginController, &LoginController::printLog, this, &MainWindow::onPrintLog);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCtrls()
{
    // 初始化求购面额数量
    QLineEdit* fvCountEdits[] = {ui->fv5CountEdit, ui->fv10CountEdit, ui->fv50CountEdit, ui->fv100CountEdit,
                         ui->fv200CountEdit, ui->fv500CountEdit, ui->fv1000CountEdit};
    QLineEdit* fvDisountEdits[] = {ui->fv5DiscountEdit, ui->fv10DiscountEdit, ui->fv50DiscountEdit, ui->fv100DiscountEdit,
                         ui->fv200DiscountEdit, ui->fv500DiscountEdit, ui->fv1000DiscountEdit};
    const QVector<BuyCouponSetting>& buyCouponSettings = SettingManager::getInstance()->m_buyCouponSetting;
    for (int i=0; i<buyCouponSettings.size(); i++)
    {
        fvCountEdits[i]->setText(QString::number(buyCouponSettings[i].m_willBuyCount));
        fvDisountEdits[i]->setText(QString::number(buyCouponSettings[i].m_discount));
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
        ChargeDialog* dlg = new ChargeDialog();
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_multiChargeController)
    {
        UiUtil::showTip(QString::fromWCharArray(L"请先取消求购"));
        event->ignore();
        return;
    }

    event->accept();
}

void MainWindow::onCtrlDShortcut()
{

}

void MainWindow::onPrintLog(QString content)
{
    static int lineCount = 0;
    if (lineCount >= 1000)
    {
        ui->logEdit->clear();
        lineCount = 0;
    }
    lineCount++;

    qInfo(content.toStdString().c_str());
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentTimeString = currentDateTime.toString("[MM-dd hh:mm:ss] ");
    QString line = currentTimeString + content;
    ui->logEdit->append(line);
}

void MainWindow::onStartBuyButtonClicked()
{
    int faceValues[] = {5, 10, 50, 100, 200, 500, 1000};
    QLineEdit* fvCountEdits[] = {ui->fv5CountEdit, ui->fv10CountEdit, ui->fv50CountEdit, ui->fv100CountEdit,
                         ui->fv200CountEdit, ui->fv500CountEdit, ui->fv1000CountEdit};
    QLineEdit* fvDisountEdits[] = {ui->fv5DiscountEdit, ui->fv10DiscountEdit, ui->fv50DiscountEdit, ui->fv100DiscountEdit,
                         ui->fv200DiscountEdit, ui->fv500DiscountEdit, ui->fv1000DiscountEdit};
    QVector<BuyCouponSetting> buyCouponSettings;
    for (int i=0; i<sizeof(faceValues)/sizeof(faceValues[0]); i++)
    {
        BuyCouponSetting setting;
        setting.m_faceVal = faceValues[i];
        if (fvCountEdits[i]->text().isEmpty())
        {
            UiUtil::showTip(QString::fromWCharArray(L"请填写正确的求购面额数量"));
            return;
        }
        else
        {
            bool ok = false;
            int count = fvCountEdits[i]->text().toInt(&ok);
            if (!ok || count < 0)
            {
                UiUtil::showTip(QString::fromWCharArray(L"请填写正确的求购面额数量"));
                return;
            }
            setting.m_willBuyCount = count;
        }

        if (fvDisountEdits[i]->text().isEmpty())
        {
            UiUtil::showTip(QString::fromWCharArray(L"请填写正确的折扣"));
            return;
        }
        else
        {
            bool ok = false;
            int discount = fvDisountEdits[i]->text().toInt(&ok);
            if (!ok || discount < 100 || discount > 1000)
            {
                UiUtil::showTip(QString::fromWCharArray(L"请填写正确的折扣"));
                return;
            }
            setting.m_discount = discount;
        }
        buyCouponSettings.append(setting);
    }

    // 校验是否有设置数量
    bool ok = false;
    for (auto& buyCouponSetting : buyCouponSettings)
    {
        if (buyCouponSetting.m_willBuyCount > 0)
        {
            ok = true;
            break;
        }
    }
    if (!ok)
    {
        UiUtil::showTip(QString::fromWCharArray(L"请填写求购面额数量"));
        return;
    }

    SettingManager::getInstance()->m_buyCouponSetting = buyCouponSettings;
    SettingManager::getInstance()->save();

    if (SettingManager::getInstance()->getTotalChargeMoney() == 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"请添加充值手机和充值金额"));
        return;
    }

    ui->startBuyButton->setEnabled(false);
    ui->cancelBuyButton->setEnabled(true);
    ui->addPhoneButton->setEnabled(false);

    m_multiChargeController = new MultiChargeController();
    connect(m_multiChargeController, &MultiChargeController::printLog, this, &MainWindow::onPrintLog);
    connect(m_multiChargeController, &MultiChargeController::chargeChange, [this]() {
        initPhoneTableView();
    });
    connect(m_multiChargeController, &MultiChargeController::runFinish, [this](bool success) {
        if (success)
        {
            onPrintLog(QString::fromWCharArray(L"求购成功"));
        }
        else
        {
            onPrintLog(QString::fromWCharArray(L"求购结束"));
        }

        ui->startBuyButton->setEnabled(true);
        ui->cancelBuyButton->setEnabled(false);
        ui->addPhoneButton->setEnabled(true);
        m_multiChargeController->deleteLater();
        m_multiChargeController = nullptr;
    });
    m_multiChargeController->run();
}

void MainWindow::onCancelBuyButtonClicked()
{
    if (m_multiChargeController)
    {
        m_multiChargeController->requestStop();
        ui->cancelBuyButton->setEnabled(false);
    }
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
