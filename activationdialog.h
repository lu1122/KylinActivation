#ifndef ACTIVATIONDIALOG_H
#define ACTIVATIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>

class ActivationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActivationDialog(const QString &serialNumber, QWidget *parent = nullptr);
    ~ActivationDialog();

    QString getSerialCode() const;
    QString getActivationCode() const;
    QString getProjectNumber() const;
    QString getChassisNumber() const;

private:
    QString serialNumber;
    
    // UI 组件
    QVBoxLayout *mainLayout;
    QFormLayout *formLayout;
    QLabel *serialCodeLabel;
    QLineEdit *serialCodeEdit;
    QLabel *activationCodeLabel;
    QLineEdit *activationCodeEdit;
    QLabel *projectNumberLabel;
    QLineEdit *projectNumberEdit;
    QLabel *chassisNumberLabel;
    QLineEdit *chassisNumberEdit;
    
    void setupUI();
};

#endif // ACTIVATIONDIALOG_H