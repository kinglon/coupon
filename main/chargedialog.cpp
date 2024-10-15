#include "chargedialog.h"
#include "ui_chargedialog.h"
#include "settingmanager.h"
#include "Utility/ImPath.h"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
#include "uiutil.h"

using namespace QXlsx;

ChargeDialog::ChargeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChargeDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    initCtrls();
}

ChargeDialog::~ChargeDialog()
{
    delete ui;
}

void ChargeDialog::initCtrls()
{
    initCouponTableView();

    connect(ui->startQueryButton, &QPushButton::clicked, [this]() {
        onStartQueryButtonClicked();
    });

    connect(ui->startBindButton, &QPushButton::clicked, [this]() {
        onStartBindButtonClicked();
    });

    connect(ui->importCouponButton, &QPushButton::clicked, [this]() {
        onImportCouponButtonClicked();
    });

    connect(ui->deleteCouponButton, &QPushButton::clicked, [this]() {
        onDeleteCouponButtonClicked();
    });
}

void ChargeDialog::initCouponTableView()
{
    ui->couponTableView->setModel(&m_couponModel);

    m_couponModel.clear();
    m_couponModel.setColumnCount(5);
    m_couponModel.setHeaderData(0, Qt::Horizontal, QString::fromWCharArray(L"卡号"));
    m_couponModel.setHeaderData(1, Qt::Horizontal, QString::fromWCharArray(L"卡密"));
    m_couponModel.setHeaderData(2, Qt::Horizontal, QString::fromWCharArray(L"面值"));
    m_couponModel.setHeaderData(3, Qt::Horizontal, QString::fromWCharArray(L"有效期"));
    m_couponModel.setHeaderData(4, Qt::Horizontal, QString::fromWCharArray(L"状态"));

    QVector<Coupon>& coupons = SettingManager::getInstance()->m_coupons;
    for (int i=0; i<coupons.size(); i++)
    {
        QStandardItem* item = new QStandardItem(coupons[i].m_couponId);
        item->setTextAlignment(Qt::AlignCenter);
        m_couponModel.setItem(i, 0, item);

        item = new QStandardItem(coupons[i].m_couponPassword);
        item->setTextAlignment(Qt::AlignCenter);
        m_couponModel.setItem(i, 1, item);

        item = new QStandardItem(QString::number(coupons[i].m_faceValue));
        item->setTextAlignment(Qt::AlignCenter);
        m_couponModel.setItem(i, 2, item);

        item = new QStandardItem(coupons[i].m_expiredDate);
        item->setTextAlignment(Qt::AlignCenter);
        m_couponModel.setItem(i, 3, item);

        item = new QStandardItem(coupons[i].m_status);
        m_couponModel.setItem(i, 4, item);
    }
}

void ChargeDialog::onImportCouponButtonClicked()
{
    QString couponFilePath = QString::fromStdWString(CImPath::GetConfPath() + L"\\CODE.xlsx");
    Document xlsx(couponFilePath);
    if (!xlsx.load())
    {
        UiUtil::showTip(QString::fromWCharArray(L"加载卡券Excel文件失败"));
        return;
    }

    QVector<Coupon> coupons;
    CellRange range = xlsx.dimension();
    for (int row=2; row <= range.lastRow(); row++)
    {
        Coupon coupon;
        Cell* cell = xlsx.cellAt(row, 1);
        if (cell)
        {
            coupon.m_couponId = cell->readValue().toString();
        }
        cell = xlsx.cellAt(row, 2);
        if (cell)
        {
            coupon.m_couponPassword = cell->readValue().toString();
        }

        if (coupon.isValid())
        {
            coupons.append(coupon);
            continue;
        }

        break;
    }

    if (coupons.size() == 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"没有卡券"));
        return;
    }

    if (SettingManager::getInstance()->m_coupons.size() > 0)
    {
        if (!UiUtil::showTipV2(QString::fromWCharArray(L"确定要重新导入卡券吗")))
        {
            return;
        }
    }

    SettingManager::getInstance()->m_coupons = coupons;
    initCouponTableView();
}

void ChargeDialog::onPrintLog(QString content)
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

void ChargeDialog::onStartQueryButtonClicked()
{
    QString mobile = ui->phoneEdit->text();
    if (mobile.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"充值手机号未填写"));
        return;
    }

    QVector<Coupon> coupons;
    for (auto& coupon : SettingManager::getInstance()->m_coupons)
    {
        if (coupon.m_faceValue == 0)
        {
            coupons.append(coupon);
        }
    }
    if (coupons.size() == 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"所有卡券都已查询"));
        return;
    }

    ui->startQueryButton->setEnabled(false);
    ui->startBindButton->setEnabled(false);

    m_couponQuerier = new CouponQuerier();
    connect(m_couponQuerier, &CouponQuerier::printLog, this, &ChargeDialog::onPrintLog);
    connect(m_couponQuerier, &CouponQuerier::oneCouponFinish, [this](Coupon newCoupon) {
        for (auto& coupon : SettingManager::getInstance()->m_coupons)
        {
            if (coupon.m_couponPassword == newCoupon.m_couponPassword)
            {
                coupon = newCoupon;
                break;
            }
        }
        initCouponTableView();
    });
    connect(m_couponQuerier, &CouponQuerier::runFinish, [this](bool) {
        ui->startQueryButton->setEnabled(true);
        ui->startBindButton->setEnabled(true);
        m_couponQuerier->deleteLater();
        m_couponQuerier = nullptr;
    });
    m_couponQuerier->run(mobile, coupons);
}

void ChargeDialog::onStartBindButtonClicked()
{
    QString mobile = ui->phoneEdit->text();
    if (mobile.isEmpty())
    {
        UiUtil::showTip(QString::fromWCharArray(L"充值手机号未填写"));
        return;
    }

    bool ok = false;
    int chargeMoney = ui->chargeMoneyEdit->text().toInt(&ok);
    if (!ok || chargeMoney < 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"充值金额未填写"));
        return;
    }

    if (SettingManager::getInstance()->m_coupons.size() == 0)
    {
        UiUtil::showTip(QString::fromWCharArray(L"没有卡券"));
        return;
    }

    ui->startQueryButton->setEnabled(false);
    ui->startBindButton->setEnabled(false);
    m_chargeController = new SingleChargeController();
    connect(m_chargeController, &SingleChargeController::printLog, this, &ChargeDialog::onPrintLog);
    connect(m_chargeController, &SingleChargeController::couponStatusChange, [this](QString couponPassword, QString status) {
        for (auto& coupon : SettingManager::getInstance()->m_coupons)
        {
            if (coupon.m_couponPassword == couponPassword)
            {
                coupon.m_status = status;
                break;
            }
        }
        initCouponTableView();
    });
    connect(m_chargeController, &SingleChargeController::runFinish, [this](bool) {
        ui->startQueryButton->setEnabled(true);
        ui->startBindButton->setEnabled(true);
        m_chargeController->deleteLater();
        m_chargeController = nullptr;
    });
    m_chargeController->run(mobile, chargeMoney, SettingManager::getInstance()->m_coupons);
}

void ChargeDialog::onDeleteCouponButtonClicked()
{
    if (!UiUtil::showTipV2(QString::fromWCharArray(L"确定要删除无效卡券吗？")))
    {
        return;
    }

    QVector<Coupon>& coupons = SettingManager::getInstance()->m_coupons;
    for (auto it=coupons.begin(); it != coupons.end();)
    {
        if (it->m_status.indexOf(QString::fromWCharArray(L"已使用")) >=0
                || it->m_status.indexOf(QString::fromWCharArray(L"未激活")) >=0
                || it->m_status.indexOf(QString::fromWCharArray(L"卡号卡密错误")) >=0)
        {
            it = coupons.erase(it);
        }
        else
        {
            it++;
        }
    }

    initCouponTableView();
}

void ChargeDialog::closeEvent(QCloseEvent *event)
{
    if (m_couponQuerier)
    {
        UiUtil::showTip(QString::fromWCharArray(L"正在查询中"));
        event->ignore();
        return;
    }

    if (m_chargeController)
    {
        UiUtil::showTip(QString::fromWCharArray(L"正在充值中"));
        event->ignore();
        return;
    }

    event->accept();
}
