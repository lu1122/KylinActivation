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
#include <QShortcut>
#include <QTextStream>

class ActivationDialog;

struct CSVData {
    QString serialNumber;//序列号
    int totalActivations = 0;//总激活次数
    int remainingActivations = 0;//剩余激活次数
    QVector<QPair<QString, QString>> activationCodes;//激活码
};

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
    void modifyChildItem();
    void deleteChildItem(const QModelIndex &index);
    void updateChildItemInDatabase(const QModelIndex &index);
    void modifySerialNumber();
    void addActivationInfo();
    void deleteSerialNumber();
    void downloadLicense();
    void downloadKyinfo();
    void importFromCSV();
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

    // 添加搜索相关成员
    QShortcut *searchShortcut;
    QDialog *searchDialog;
    QLineEdit *searchEdit;
    QPushButton *searchNextButton;
    QPushButton *searchPrevButton;
    QComboBox *searchFieldCombo;
    QList<QModelIndex> searchResults;
    int currentSearchIndex;

    // 添加搜索方法
    void setupSearchDialog();
    void performSearch();
    void highlightSearchResult(int index);
    void findNext();
    void findPrev();
    void clearSearchHighlights();

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

    //Excel数据
    CSVData parseCSVFile(const QString &filePath);
    bool addDataToSystem(const CSVData &data);
    bool isSerialNumberExists(const QString &serialNumber);
};

#endif // MAINWINDOW_H
