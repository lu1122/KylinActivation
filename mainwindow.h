#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QStandardItemModel>
#include <QMap>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>

class ActivationDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addSerialNumber();
    void platformChanged(int index);
    void bindWechatChanged(int index);
    void uploadLicense();
    void uploadKyinfo();
    void showSerialContextMenu(const QPoint &pos);
    void modifySerialNumber();
    void addActivationInfo();
    void deleteSerialNumber();
    void downloadLicense();
    void downloadKyinfo();

private:
    // UI 组件
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    // 添加序列号区域
    QGroupBox *addSerialGroup;
    QGridLayout *serialFormLayout;
    QLabel *serialNumberLabel;
    QLineEdit *serialNumberEdit;
    QLabel *totalActivationsLabel;
    QLineEdit *totalActivationsEdit;
    QLabel *remainingActivationsLabel;
    QLineEdit *remainingActivationsEdit;
    QLabel *platformLabel;
    QComboBox *platformComboBox;
    QLabel *verificationCodeLabel;
    QLineEdit *verificationCodeEdit;
    QLabel *licenseFileLabel;
    QPushButton *uploadLicenseButton;
    QLabel *licenseFilePathLabel;
    QLabel *kyinfoFileLabel;
    QPushButton *uploadKyinfoButton;
    QLabel *kyinfoFilePathLabel;
    QLabel *bindWechatLabel;
    QComboBox *bindWechatComboBox;
    QLabel *bindPersonLabel;
    QLineEdit *bindPersonEdit;
    QPushButton *addButton;
    
    // 序列号表格区域
    QGroupBox *serialTableGroup;
    QVBoxLayout *serialTableLayout;
    QTreeView *serialTableView;
    
    // 数据模型
    QStandardItemModel *serialModel;
    QMap<QString, QStandardItemModel*> activationModels;
    ActivationDialog *activationDialog;

    // 数据库
    QSqlDatabase db;

    // 方法
    bool initDatabase();
    void loadSerialNumbers();
    void loadActivationInfo();
    bool verifyPassword();
    void updateSerialNumberInDatabase(const QModelIndex &index);
    void updateActivationInfoInDatabase(const QString &serialNumber, const QModelIndex &index);
    
    void setupUI();
    void setupSerialForm();
    void setupSerialTable();
};

#endif // MAINWINDOW_H