#ifndef MULTICHARGECONTROLLER_H
#define MULTICHARGECONTROLLER_H

#include <QObject>

// 自动求购充值
class MultiChargeController : public QObject
{
    Q_OBJECT
public:
    explicit MultiChargeController(QObject *parent = nullptr);

    void run();

    void requestStop();

signals:
    void runFinish(bool success);

    void printLog(QString content);
};

#endif // MULTICHARGECONTROLLER_H
