#include "uiutil.h"
#include <QMessageBox>

void UiUtil::showTip(QString tip)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(QString::fromWCharArray(L"提示"));
    msgBox.setText(tip);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void UiUtil::showImage(const QPixmap& image, QLabel& labelCtrl)
{
    QSize size = labelCtrl.size();
    QPixmap pixmap = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    labelCtrl.setPixmap(pixmap);
    labelCtrl.setAlignment(Qt::AlignCenter);
}
