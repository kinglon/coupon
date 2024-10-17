#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QObject>

class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(QObject *parent = nullptr);

signals:
    void printLog(QString logContent);

private slots:
    void onMainTimer();

private:
    qint64 m_lastRefreshTime = 0;
};

#endif // LOGINCONTROLLER_H
