#include "activationdialog.h"

ActivationDialog::ActivationDialog(const QString &serialNumber, QWidget *parent)
    : QDialog(parent), serialNumber(serialNumber)
{
    setupUI();
    setWindowTitle("添加激活信息 - " + serialNumber);
}

ActivationDialog::~ActivationDialog()
{
}

void ActivationDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout();
    
    serialCodeLabel = new QLabel("序列码:", this);
    serialCodeEdit = new QLineEdit(this);
    activationCodeLabel = new QLabel("激活码:", this);
    activationCodeEdit = new QLineEdit(this);
    projectNumberLabel = new QLabel("项目号:", this);
    projectNumberEdit = new QLineEdit(this);
    chassisNumberLabel = new QLabel("机箱序列号:", this);
    chassisNumberEdit = new QLineEdit(this);
    
    formLayout->addRow(serialCodeLabel, serialCodeEdit);
    formLayout->addRow(activationCodeLabel, activationCodeEdit);
    formLayout->addRow(projectNumberLabel, projectNumberEdit);
    formLayout->addRow(chassisNumberLabel, chassisNumberEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString ActivationDialog::getSerialCode() const
{
    return serialCodeEdit->text().trimmed();
}

QString ActivationDialog::getActivationCode() const
{
    return activationCodeEdit->text().trimmed();
}

QString ActivationDialog::getProjectNumber() const
{
    return projectNumberEdit->text().trimmed();
}

QString ActivationDialog::getChassisNumber() const
{
    return chassisNumberEdit->text().trimmed();
}