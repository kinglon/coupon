#include "chargedialog.h"
#include "ui_chargedialog.h"

ChargeDialog::ChargeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChargeDialog)
{
    ui->setupUi(this);
}

ChargeDialog::~ChargeDialog()
{
    delete ui;
}
