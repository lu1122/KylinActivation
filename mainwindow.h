#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTableView>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddButtonClicked();
    void onDeleteButtonClicked();
    void onExportButtonClicked();
    void onImportButtonClicked();
    void onBrowseLicenseClicked();
    void onBrowseKyinfoClicked();

private:
    void initDatabase();
    void setupUI();
    void setupTableView();
    void loadData();
    bool addActivationCode(const QString &serial, const QString &code, 
                                 const QString &project, const QString &chassis,
                                 const QString &licenseFilePath, const QString &kyinfoFilePath);

    // UI 组件
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    // 输入区域
    QGroupBox *inputGroup;
    QGridLayout *inputLayout;
    
    QLabel *serialLabel;
    QLineEdit *serialEdit;
    
    QLabel *codeLabel;
    QLineEdit *codeEdit;
    
    QLabel *projectLabel;
    QLineEdit *projectEdit;
    
    QLabel *chassisLabel;
    QLineEdit *chassisEdit;
    
    QLabel *licenseLabel;
    QLineEdit *licenseEdit;
    QPushButton *browseLicenseButton;
    
    QLabel *kyinfoLabel;
    QLineEdit *kyinfoEdit;
    QPushButton *browseKyinfoButton;
    
    // 按钮区域
    QHBoxLayout *buttonLayout;
    QPushButton *addButton;
    QPushButton *deleteButton;
    QPushButton *exportButton;
    QPushButton *importButton;
    
    // 数据显示区域
    QTableView *tableView;
    
    // 数据库相关
    QSqlDatabase db;
    QSqlTableModel *model;
};

#endif // MAINWINDOW_H