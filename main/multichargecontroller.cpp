#include "multichargecontroller.h"
#include "settingmanager.h"
#include "orderstatusreporter.h"

MultiChargeController::MultiChargeController(QObject *parent)
    : QObject{parent}
{

}

void MultiChargeController::run()
{
    m_couponBuyer = new CouponBuyer();
    connect(m_couponBuyer, &CouponBuyer::printLog, this, &MultiChargeController::printLog);
    connect(m_couponBuyer, &CouponBuyer::haveNewCoupon, [this](QVector<GetCouponResult> coupons) {
        m_coupons.append(coupons);
        doCharge();
    });
    connect(m_couponBuyer, &CouponBuyer::runFinish, [this]() {
        m_couponBuyer->deleteLater();
        m_couponBuyer = nullptr;
        requestStop();
    });
    m_couponBuyer->run();
}

void MultiChargeController::doCharge()
{
    if (m_chargeController)
    {
        return;
    }

    if (m_coupons.size() == 0)
    {
        return;
    }

    // 充值手机按优先级排序
    QVector<ChargePhone> chargePhones = SettingManager::getInstance()->m_chargePhones;
    std::sort(chargePhones.begin(), chargePhones.end(), [](ChargePhone& a, ChargePhone& b) {
        return a.m_priority < b.m_priority;
    });

    // 卡券按面额从小到大排序
    std::sort(m_coupons.begin(), m_coupons.end(), [](GetCouponResult& a, GetCouponResult& b) {
        return a.m_coupon.m_faceValue < b.m_coupon.m_faceValue;
    });

    for (auto& chargePhone : chargePhones)
    {
        int needChargeMoney = chargePhone.m_moneyCount - chargePhone.m_chargeMoney;
        QVector<Coupon> coupons;
        int totalCouponMoney = 0;
        for (auto& coupon : m_coupons)
        {
            if (coupon.m_coupon.m_faceValue <= needChargeMoney)
            {
                coupons.append(coupon.m_coupon);
                needChargeMoney -= coupon.m_coupon.m_faceValue;
                totalCouponMoney += coupon.m_coupon.m_faceValue;
                continue;
            }
            break;
        }

        if (coupons.size() > 0)
        {
            doCharge(chargePhone.m_phoneNumber, totalCouponMoney, coupons);
            break;
        }
    }
}

void MultiChargeController::doCharge(QString mobile, int chargeMoney, const QVector<Coupon>& coupons)
{
    m_chargeController = new SingleChargeController();
    connect(m_chargeController, &SingleChargeController::printLog, this, &MultiChargeController::printLog);
    connect(m_chargeController, &SingleChargeController::chargeCompletely, [this, mobile](ChargeResult result) {
        for (auto it = m_coupons.begin(); it != m_coupons.end(); it++)
        {
            if (it->m_coupon.m_couponPassword == result.m_coupon.m_couponPassword)
            {
                // 上报订单状态
                OrderStatusReporter::getInstance()->reportOrderStatus(it->m_recordId, it->m_orderId, result);

                m_coupons.erase(it);
                break;
            }
        }

        // 更新手机已充金额
        for (auto& chargePhone : SettingManager::getInstance()->m_chargePhones)
        {
            if (chargePhone.m_phoneNumber == mobile)
            {
                chargePhone.m_chargeMoney += result.m_realFaceValue;
                break;
            }
        }
        emit chargeChange();

        // 可能有新的卡券到来，继续充值
        doCharge();
    });
    connect(m_chargeController, &SingleChargeController::runFinish, [this](bool) {        
        m_chargeController->deleteLater();
        m_chargeController = nullptr;       

        if (isNeedCharge())
        {
            doCharge();
        }
        else
        {
            requestStop();
        }
    });
    m_chargeController->run(mobile, chargeMoney, coupons);
}

bool MultiChargeController::isNeedCharge()
{
    bool needCharge = false;
    for (auto& chargePhone : SettingManager::getInstance()->m_chargePhones)
    {
        if (chargePhone.m_moneyCount > chargePhone.m_chargeMoney)
        {
            needCharge = true;
            break;
        }
    }
    return needCharge;
}

void MultiChargeController::requestStop()
{
    if (m_couponBuyer == nullptr && m_chargeController == nullptr)
    {
        emit runFinish(!isNeedCharge());
        return;
    }

    if (m_couponBuyer)
    {
        m_couponBuyer->requestStop();
    }
}
