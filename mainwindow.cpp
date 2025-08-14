#include "mainwindow.h"
#include "activationdialog.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QMenu>
#include <QInputDialog>
#include <QStandardItem>
#include <QHeaderView>
#include <QTimer>
#include <QSqlError>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // 初始化数据库
    if (!initDatabase()) {
        QMessageBox::critical(this, "错误", "无法初始化数据库!");
        QTimer::singleShot(0, this, &QMainWindow::close);
        return;
    }

    // 加载数据
    loadSerialNumbers();
    loadActivationInfo();

    // 初始化搜索相关
    setupSearchDialog();
    currentSearchIndex = -1;
}

MainWindow::~MainWindow()
{
    db.close();
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    
    setupSerialForm();
    setupSerialTable();
    
    setCentralWidget(centralWidget);
    resize(1000, 600);
    setWindowTitle("银河麒麟激活码管理平台");

    // 添加快捷键
    searchShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
    connect(searchShortcut, &QShortcut::activated, [this]() {
        qDebug() << "Ctrl+F pressed";  // 调试输出
        searchDialog->show();
        searchEdit->setFocus();
    });

    // 添加导入按钮
    QPushButton *importButton = new QPushButton("从CSV导入", this);
    mainLayout->addWidget(importButton);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importFromCSV);
}

void MainWindow::setupSerialForm()
{
    addSerialGroup = new QGroupBox("添加序列号", centralWidget);
    serialFormLayout = new QGridLayout(addSerialGroup);
    
    // 第一行
    serialNumberLabel = new QLabel("序列号:", addSerialGroup);
    serialNumberEdit = new QLineEdit(addSerialGroup);
    totalActivationsLabel = new QLabel("总激活次数:", addSerialGroup);
    totalActivationsEdit = new QLineEdit(addSerialGroup);
    remainingActivationsLabel = new QLabel("剩余次数:", addSerialGroup);
    remainingActivationsEdit = new QLineEdit(addSerialGroup);
    
    serialFormLayout->addWidget(serialNumberLabel, 0, 0);
    serialFormLayout->addWidget(serialNumberEdit, 0, 1);
    serialFormLayout->addWidget(totalActivationsLabel, 0, 2);
    serialFormLayout->addWidget(totalActivationsEdit, 0, 3);
    serialFormLayout->addWidget(remainingActivationsLabel, 0, 4);
    serialFormLayout->addWidget(remainingActivationsEdit, 0, 5);
    
    // 第二行
    platformLabel = new QLabel("硬件平台:", addSerialGroup);
    platformComboBox = new QComboBox(addSerialGroup);
    platformComboBox->addItems({"银河麒麟", "飞腾"});
    verificationCodeLabel = new QLabel("验证码:", addSerialGroup);
    verificationCodeEdit = new QLineEdit(addSerialGroup);
    verificationCodeEdit->setEnabled(false);
    
    serialFormLayout->addWidget(platformLabel, 1, 0);
    serialFormLayout->addWidget(platformComboBox, 1, 1);
    serialFormLayout->addWidget(verificationCodeLabel, 1, 2);
    serialFormLayout->addWidget(verificationCodeEdit, 1, 3);
    
    // 第三行
    licenseFileLabel = new QLabel("LICENSE文件:", addSerialGroup);
    uploadLicenseButton = new QPushButton("上传", addSerialGroup);
    licenseFilePathLabel = new QLabel("未选择文件", addSerialGroup);
    kyinfoFileLabel = new QLabel(".kyinfo文件:", addSerialGroup);
    uploadKyinfoButton = new QPushButton("上传", addSerialGroup);
    kyinfoFilePathLabel = new QLabel("未选择文件", addSerialGroup);
    
    serialFormLayout->addWidget(licenseFileLabel, 2, 0);
    serialFormLayout->addWidget(uploadLicenseButton, 2, 1);
    serialFormLayout->addWidget(licenseFilePathLabel, 2, 2, 1, 2);
    serialFormLayout->addWidget(kyinfoFileLabel, 2, 4);
    serialFormLayout->addWidget(uploadKyinfoButton, 2, 5);
    serialFormLayout->addWidget(kyinfoFilePathLabel, 2, 6, 1, 2);
    
    // 第四行
    bindWechatLabel = new QLabel("绑定微信:", addSerialGroup);
    bindWechatComboBox = new QComboBox(addSerialGroup);
    bindWechatComboBox->addItems({"是", "否"});
    bindPersonLabel = new QLabel("绑定人:", addSerialGroup);
    bindPersonEdit = new QLineEdit(addSerialGroup);
    bindPersonEdit->setEnabled(true);
    
    serialFormLayout->addWidget(bindWechatLabel, 3, 0);
    serialFormLayout->addWidget(bindWechatComboBox, 3, 1);
    serialFormLayout->addWidget(bindPersonLabel, 3, 2);
    serialFormLayout->addWidget(bindPersonEdit, 3, 3);
    
    // 添加按钮
    addButton = new QPushButton("添加", addSerialGroup);
    serialFormLayout->addWidget(addButton, 4, 0, 1, 8);
    
    mainLayout->addWidget(addSerialGroup);
    
    // 连接信号槽
    connect(platformComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::platformChanged);
    connect(bindWechatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::bindWechatChanged);
    connect(uploadLicenseButton, &QPushButton::clicked, this, &MainWindow::uploadLicense);
    connect(uploadKyinfoButton, &QPushButton::clicked, this, &MainWindow::uploadKyinfo);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addSerialNumber);
}

void MainWindow::setupSerialTable()
{
    serialTableGroup = new QGroupBox("序列号列表", centralWidget);
    serialTableLayout = new QVBoxLayout(serialTableGroup);
    
    serialTableView = new QTreeView(serialTableGroup);
    serialModel = new QStandardItemModel(this);
    serialModel->setHorizontalHeaderLabels({"序列号", "总激活次数", "剩余次数", "硬件平台", 
                                          "验证码", "LICENSE", ".kyinfo", "绑定微信", "绑定人", "激活码", "项目号", "机箱序列号"});
    serialTableView->setModel(serialModel);
//    serialTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    serialTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    serialTableLayout->addWidget(serialTableView);
    mainLayout->addWidget(serialTableGroup);
    
    // 连接信号槽
    connect(serialTableView, &QTreeView::customContextMenuRequested, this, &MainWindow::showSerialContextMenu);
    // 禁用双击编辑
    serialTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

bool MainWindow::initDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("kylin_activation.db");

    if (!db.open()) {
        return false;
    }

    // 创建序列号表
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS serial_numbers ("
                   "serial_number TEXT PRIMARY KEY, "
                   "total_activations INTEGER, "
                   "remaining_activations INTEGER, "
                   "platform TEXT, "
                   "verification_code TEXT, "
                   "license_file BLOB, "
                   "kyinfo_file BLOB, "
                   "bind_wechat TEXT, "
                   "bind_person TEXT)")) {
        return false;
    }

    // 创建激活信息表
    if (!query.exec("CREATE TABLE IF NOT EXISTS activation_info ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "serial_number TEXT, "
                   "activation_code TEXT, "
                   "project_number TEXT, "
                   "chassis_number TEXT, "
                   "FOREIGN KEY(serial_number) REFERENCES serial_numbers(serial_number))")) {
        return false;
    }

    return true;
}

void MainWindow::loadSerialNumbers()
{
    serialModel->removeRows(0, serialModel->rowCount());

    QSqlQuery query("SELECT * FROM serial_numbers");
    while (query.next()) {
        QList<QStandardItem*> items;
        items << new QStandardItem(query.value("serial_number").toString());
        items << new QStandardItem(query.value("total_activations").toString());
        items << new QStandardItem(query.value("remaining_activations").toString());
        items << new QStandardItem(query.value("platform").toString());
        items << new QStandardItem(query.value("verification_code").toString());
        items << new QStandardItem(query.value("license_file").isNull() ? "无" : "有");
        items << new QStandardItem(query.value("kyinfo_file").isNull() ? "无" : "有");
        items << new QStandardItem(query.value("bind_wechat").toString());
        items << new QStandardItem(query.value("bind_person").toString());
        
        serialModel->appendRow(items);
    }
}

void MainWindow::loadActivationInfo()
{
     // 遍历所有激活信息，添加到对应主行下作为子项
    QSqlQuery query("SELECT * FROM activation_info");
    while (query.next()) {
        QString serialNumber = query.value("serial_number").toString();
        QString activationCode = query.value("activation_code").toString();
        QString projectNumber = query.value("project_number").toString();
        QString chassisNumber = query.value("chassis_number").toString();

        // 查找对应的主行（顶层项）
        QStandardItem *parentItem = nullptr;
        for (int row = 0; row < serialModel->rowCount(); ++row) {
            QStandardItem *item = serialModel->item(row, 0);  // 主行的序列号列
            if (item && item->text() == serialNumber) {
                parentItem = item;  // 找到主行的第一项（作为父项）
                break;
            }
        }

        if (parentItem) {
            QList<QStandardItem*> childItems;
            // 创建子项（12列，比主模型多激活码、项目号、机箱号）
            for (int i = 0; i < 9; ++i) {
                childItems << new QStandardItem("");  // 填充剩余列
            }
            childItems << new QStandardItem(activationCode);    // 列0
            childItems << new QStandardItem(projectNumber);     // 列1
            childItems << new QStandardItem(chassisNumber);     // 列2
            
            // 添加到主行下
            parentItem->appendRow(childItems);
        }
    }
}

void MainWindow::setupSearchDialog()
{
    searchDialog = new QDialog(this);
    searchDialog->setWindowTitle("搜索");
    searchDialog->setModal(false);
    
    QVBoxLayout *layout = new QVBoxLayout(searchDialog);
    
    // 搜索字段选择
    QHBoxLayout *fieldLayout = new QHBoxLayout();
    QLabel *fieldLabel = new QLabel("搜索字段:", searchDialog);
    searchFieldCombo = new QComboBox(searchDialog);
    searchFieldCombo->addItems({"序列号", "激活码", "项目号", "机箱序列号"});
    fieldLayout->addWidget(fieldLabel);
    fieldLayout->addWidget(searchFieldCombo);
    
    // 搜索输入框
    searchEdit = new QLineEdit(searchDialog);
    
    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    searchNextButton = new QPushButton("下一个", searchDialog);
    searchPrevButton = new QPushButton("上一个", searchDialog);
    // QPushButton *closeButton = new QPushButton("关闭", searchDialog);
    buttonLayout->addWidget(searchPrevButton);
    buttonLayout->addWidget(searchNextButton);
    // buttonLayout->addWidget(closeButton);
    
    layout->addLayout(fieldLayout);
    layout->addWidget(searchEdit);
    layout->addLayout(buttonLayout);
    
    // 添加文本变化实时搜索
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::performSearch);
    // 连接信号槽
    connect(searchEdit, &QLineEdit::returnPressed, this, &MainWindow::performSearch);
    connect(searchNextButton, &QPushButton::clicked, this, &MainWindow::findNext);
    connect(searchPrevButton, &QPushButton::clicked, this, &MainWindow::findPrev);
    // connect(closeButton, &QPushButton::clicked, searchDialog, &QDialog::hide);
    // 添加对话框关闭事件处理
    connect(searchDialog, &QDialog::finished, this, [this](int result) {
        Q_UNUSED(result);
        clearSearchHighlights();  // 对话框关闭时清除高亮
    });
}

void MainWindow::performSearch()
{
    QString searchText = searchEdit->text().trimmed();
    
    // 即使搜索内容为空也清除之前的高亮
    clearSearchHighlights();
    
    if (searchText.isEmpty()) {
        return;
    }
    
    QString field = searchFieldCombo->currentText();
    int column = -1;
    
    if (field == "序列号") column = 0;
    else if (field == "激活码") column = 9;
    else if (field == "项目号") column = 10;
    else if (field == "机箱序列号") column = 11;
    
    if (column == -1) return;
    
    // 重新搜索前清空结果
    searchResults.clear();
    currentSearchIndex = -1;
    
    // 执行搜索（包括主行和子行）
    for (int i = 0; i < serialModel->rowCount(); ++i) {
        QStandardItem *parentItem = serialModel->item(i);
        
        // 搜索主行
        if (column < 9) {
            QStandardItem *item = serialModel->item(i, column);
            if (item && item->text().contains(searchText, Qt::CaseInsensitive)) {
                searchResults.append(serialModel->index(i, column));
            }
        }
        
        // 搜索子行
        for (int j = 0; j < parentItem->rowCount(); ++j) {
            if (column >= 9) {
                QStandardItem *childItem = parentItem->child(j, column-9);
                if (childItem && childItem->text().contains(searchText, Qt::CaseInsensitive)) {
                    searchResults.append(childItem->index());
                }
            }
        }
    }
    
    if (!searchResults.isEmpty()) {
        currentSearchIndex = 0;
        highlightSearchResult(currentSearchIndex);
    }
}

void MainWindow::highlightSearchResult(int index)
{
    if (index < 0 || index >= searchResults.size()) {
        return;
    }
    
    // 清除所有高亮
    clearSearchHighlights();
    
    // 设置新的高亮
    QModelIndex resultIndex = searchResults[index];
    QStandardItem *item = serialModel->itemFromIndex(resultIndex);
    if (item) {
        item->setBackground(Qt::yellow);
        
        // 确保该项可见
        serialTableView->scrollTo(resultIndex);
        serialTableView->selectionModel()->select(resultIndex, QItemSelectionModel::SelectCurrent);
        
        // 如果是子项，展开父项
        if (resultIndex.parent().isValid()) {
            serialTableView->expand(resultIndex.parent());
        }
    }
}

void MainWindow::findNext()
{
    if (searchResults.isEmpty()) {
        performSearch();
        return;
    }
    
    currentSearchIndex = (currentSearchIndex + 1) % searchResults.size();
    highlightSearchResult(currentSearchIndex);
}

void MainWindow::findPrev()
{
    if (searchResults.isEmpty()) {
        performSearch();
        return;
    }
    
    currentSearchIndex = (currentSearchIndex - 1 + searchResults.size()) % searchResults.size();
    highlightSearchResult(currentSearchIndex);
}

void MainWindow::clearSearchHighlights()
{
    /// 只清除实际有高亮的部分，提高性能
    for (const QModelIndex &index : searchResults) {
        QStandardItem *item = serialModel->itemFromIndex(index);
        if (item) {
            item->setBackground(QBrush());
        }
    }
}

void MainWindow::addSerialNumber()
{
    QString serialNumber = serialNumberEdit->text().trimmed();
    // 检查序列号是否已存在
    if (isSerialNumberExists(serialNumber)) {
        QMessageBox::warning(this, "警告", "该序列号已存在！");
        return;
    }
    QString totalActivations = totalActivationsEdit->text().trimmed();
    QString remainingActivations = remainingActivationsEdit->text().trimmed();
    QString platform = platformComboBox->currentText();
    QString verificationCode = verificationCodeEdit->text().trimmed();
    QString bindWechat = bindWechatComboBox->currentText();
    QString bindPerson = bindPersonEdit->text().trimmed();

    if (serialNumber.isEmpty() || totalActivations.isEmpty() || remainingActivations.isEmpty()) {
        QMessageBox::warning(this, "警告", "请填写完整信息!");
        return;
    }

    if (platform == "飞腾" && verificationCode.isEmpty()) {
        QMessageBox::warning(this, "警告", "飞腾平台需要填写验证码!");
        return;
    }

    if (bindWechat == "是" && bindPerson.isEmpty()) {
        QMessageBox::warning(this, "警告", "绑定微信需要填写绑定人!");
        return;
    }

    // 检查序列号是否已存在
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM serial_numbers WHERE serial_number = ?");
    checkQuery.addBindValue(serialNumber);
    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(this, "警告", "该序列号已存在!");
        return;
    }

    // 插入数据库
    QSqlQuery query;
    query.prepare("INSERT INTO serial_numbers (serial_number, total_activations, remaining_activations, "
                 "platform, verification_code, license_file, kyinfo_file, bind_wechat, bind_person) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(serialNumber);
    query.addBindValue(totalActivations.toInt());
    query.addBindValue(remainingActivations.toInt());
    query.addBindValue(platform);
    query.addBindValue(verificationCode);
    
    // 处理LICENSE文件
    QByteArray licenseData;
    if (platform == "银河麒麟" && licenseFilePathLabel->text() != "未选择文件") {
        QFile file(licenseFilePathLabel->text());
        if (file.open(QIODevice::ReadOnly)) {
            licenseData = file.readAll();
            file.close();
        }
    }
    query.addBindValue(licenseData);
    
    // 处理.kyinfo文件
    QByteArray kyinfoData;
    if (platform == "银河麒麟" && kyinfoFilePathLabel->text() != "未选择文件") {
        QFile file(kyinfoFilePathLabel->text());
        if (file.open(QIODevice::ReadOnly)) {
            kyinfoData = file.readAll();
            file.close();
        }
    }
    query.addBindValue(kyinfoData);
    
    query.addBindValue(bindWechat);
    query.addBindValue(bindPerson);

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "添加序列号失败: " + query.lastError().text());
        return;
    }

    // 更新UI
    QList<QStandardItem*> items;
    items << new QStandardItem(serialNumber);
    items << new QStandardItem(totalActivations);
    items << new QStandardItem(remainingActivations);
    items << new QStandardItem(platform);
    items << new QStandardItem(verificationCode);
    items << new QStandardItem(licenseData.isEmpty() ? "无" : "有");
    items << new QStandardItem(kyinfoData.isEmpty() ? "无" : "有");
    items << new QStandardItem(bindWechat);
    items << new QStandardItem(bindPerson);
    
    serialModel->appendRow(items);

    // 清空输入
    serialNumberEdit->clear();
    totalActivationsEdit->clear();
    remainingActivationsEdit->clear();
    verificationCodeEdit->clear();
    licenseFilePathLabel->setText("未选择文件");
    kyinfoFilePathLabel->setText("未选择文件");
    bindPersonEdit->clear();
}

void MainWindow::platformChanged(int index)
{
    bool isKylin = (platformComboBox->currentText() == "银河麒麟");
    
    verificationCodeEdit->setEnabled(!isKylin);
    uploadLicenseButton->setEnabled(isKylin);
    uploadKyinfoButton->setEnabled(isKylin);
    
    if (!isKylin) {
        licenseFilePathLabel->setText("未选择文件");
        kyinfoFilePathLabel->setText("未选择文件");
    }
}

void MainWindow::bindWechatChanged(int index)
{
    bindPersonEdit->setEnabled(bindWechatComboBox->currentText() == "是");
    if (bindWechatComboBox->currentText() == "否") {
        bindPersonEdit->clear();
    }
}

void MainWindow::uploadLicense()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择LICENSE文件"/*, "", "License Files (*.lic *.license)"*/);
    if (!filePath.isEmpty()) {
        licenseFilePathLabel->setText(filePath);
    }
}

void MainWindow::uploadKyinfo()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择.kyinfo文件"/*, "", "Kyinfo Files (*.kyinfo)"*/);
    if (!filePath.isEmpty()) {
        kyinfoFilePathLabel->setText(filePath);
    }
}

void MainWindow::showSerialContextMenu(const QPoint &pos)
{
    QModelIndex index = serialTableView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu contextMenu(this);
    bool isTopLevel = !index.parent().isValid(); // 判断是否是主行

    if (isTopLevel) {
        // 主行菜单项
        QAction *modifyAction = new QAction("修改", this);
        connect(modifyAction, &QAction::triggered, this, &MainWindow::modifySerialNumber);
        contextMenu.addAction(modifyAction);

        QAction *addAction = new QAction("增加激活信息", this);
        connect(addAction, &QAction::triggered, this, &MainWindow::addActivationInfo);
        contextMenu.addAction(addAction);

        QAction *deleteAction = new QAction("删除主行", this);
        connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSerialNumber);
        contextMenu.addAction(deleteAction);

        QAction *downloadLicenseAction = new QAction("下载LICENSE", this);
        connect(downloadLicenseAction, &QAction::triggered, this, &MainWindow::downloadLicense);
        contextMenu.addAction(downloadLicenseAction);

        QAction *downloadKyinfoAction = new QAction("下载.kyinfo", this);
        connect(downloadKyinfoAction, &QAction::triggered, this, &MainWindow::downloadKyinfo);
        contextMenu.addAction(downloadKyinfoAction);
    } else {
        // 子行菜单项
        QAction *modifyChildAction = new QAction("修改子项", this);
        connect(modifyChildAction, &QAction::triggered, this, &MainWindow::modifyChildItem);
        contextMenu.addAction(modifyChildAction);

        QAction *deleteChildAction = new QAction("删除子项", this);
        connect(deleteChildAction, &QAction::triggered, this, [this, index]() {
            deleteChildItem(index);
        });
        contextMenu.addAction(deleteChildAction);
    }

    contextMenu.exec(serialTableView->viewport()->mapToGlobal(pos));
}


void MainWindow::modifyChildItem()
{
    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid() || !index.parent().isValid()) return;

    bool ok;
    QString newValue = QInputDialog::getText(this, "修改子项", "输入新值:", 
                                          QLineEdit::Normal, index.data().toString(), &ok);
    if (ok && !newValue.isEmpty()) {
        serialModel->itemFromIndex(index)->setText(newValue);
        updateChildItemInDatabase(index);
    }
}

void MainWindow::deleteChildItem(const QModelIndex &index)
{
    // 1. 先验证密码
    if (!verifyPassword()) {
        return;
    }
    
    if (!index.isValid() || !index.parent().isValid()) {
        qDebug() << "无效的索引或不是子项";
        return;
    }

    // 获取父项(主行)的序列号
    QStandardItem *parentItem = serialModel->itemFromIndex(index.parent());
    if (!parentItem) {
        qDebug() << "无法获取父项";
        return;
    }

    QString serialNumber = parentItem->text();
    
    // 获取子项的激活码（假设激活码在第9列，根据实际调整）
    QStandardItem *childItem = serialModel->itemFromIndex(index.sibling(index.row(), 9));
    if (!childItem) {
        qDebug() << "无法获取子项的激活码";
        return;
    }
    QString activationCode = childItem->text();

    // 开始事务
    QSqlDatabase::database().transaction();

    try {
        // 1. 从数据库删除激活信息
        QSqlQuery deleteQuery;
        deleteQuery.prepare("DELETE FROM activation_info WHERE serial_number = ? AND activation_code = ?");
        deleteQuery.addBindValue(serialNumber);
        deleteQuery.addBindValue(activationCode);
        
        if (!deleteQuery.exec()) {
            throw std::runtime_error("删除激活信息失败: " + deleteQuery.lastError().text().toStdString());
        }

        // 2. 更新主行的剩余激活次数（+1）
        int remaining = serialModel->item(index.parent().row(), 2)->text().toInt() + 1;
        serialModel->item(index.parent().row(), 2)->setText(QString::number(remaining));
        
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE serial_numbers SET remaining_activations = ? WHERE serial_number = ?");
        updateQuery.addBindValue(remaining);
        updateQuery.addBindValue(serialNumber);
        
        if (!updateQuery.exec()) {
            throw std::runtime_error("更新剩余激活次数失败: " + updateQuery.lastError().text().toStdString());
        }

        // 3. 从界面删除子项
        parentItem->removeRow(index.row());

        // 提交事务
        if (!QSqlDatabase::database().commit()) {
            throw std::runtime_error("提交事务失败");
        }

        qDebug() << "成功删除子项:" << activationCode << "序列号:" << serialNumber;

    } catch (const std::exception &e) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "错误", QString::fromStdString(e.what()));
        qDebug() << "删除子项时出错:" << e.what();
    }
}

void MainWindow::updateChildItemInDatabase(const QModelIndex &index)
{
    if (!index.isValid() || !index.parent().isValid()) return;

    QString serialNumber = serialModel->itemFromIndex(index.parent())->text();
    QString oldActivationCode = serialModel->itemFromIndex(index.sibling(index.row(), 0))->text();
    QString columnName;
    
    switch (index.column()) {
    case 0: columnName = "activation_code"; break;
    case 1: columnName = "project_number"; break;
    case 2: columnName = "chassis_number"; break;
    default: return;
    }
    
    QSqlQuery query;
    query.prepare(QString("UPDATE activation_info SET %1 = ? WHERE serial_number = ? AND activation_code = ?")
                 .arg(columnName));
    query.addBindValue(index.data().toString());
    query.addBindValue(serialNumber);
    query.addBindValue(oldActivationCode);
    query.exec();
}

void MainWindow::modifySerialNumber()
{
    if (!verifyPassword()) {
        return;
    }

    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) return;

    bool ok;
    QString newValue = QInputDialog::getText(this, "修改", "输入新值:", QLineEdit::Normal, 
                                           index.data().toString(), &ok);
    if (ok && !newValue.isEmpty()) {
        serialModel->itemFromIndex(index)->setText(newValue);
        updateSerialNumberInDatabase(index);
    }
}

void MainWindow::addActivationInfo()
{
    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) {
        qDebug() << "无效的索引";
        return;
    }

    // 确保选中的是顶层主行（不是子项）
    if (index.parent().isValid()) {
        QMessageBox::warning(this, "提示", "请选择主行添加激活信息");
        return;
    }

    QStandardItem *parentItem = serialModel->itemFromIndex(index);
    if (!parentItem) {
        qDebug() << "无法获取父项";
        return;
    }

    QString serialNumber = serialModel->item(index.row(), 0)->text();
    activationDialog = new ActivationDialog(serialNumber, this);

    if (activationDialog->exec() == QDialog::Accepted) {
        // 开始事务
        QSqlDatabase::database().transaction();

        // 1. 数据库插入
        QSqlQuery query;
        query.prepare("INSERT INTO activation_info (serial_number, activation_code, project_number, chassis_number) "
                    "VALUES (?, ?, ?, ?)");
        query.addBindValue(serialNumber);
        query.addBindValue(activationDialog->getActivationCode());
        query.addBindValue(activationDialog->getProjectNumber());
        query.addBindValue(activationDialog->getChassisNumber());
        
        if (!query.exec()) {
            QSqlDatabase::database().rollback();
            QMessageBox::critical(this, "错误", "添加激活信息失败: " + query.lastError().text());
            qDebug() << "数据库插入失败:" << query.lastError().text();
            delete activationDialog;
            return;
        }

        // 2. 添加子项（共9列，与主模型保持一致）
        QList<QStandardItem*> childItems;
        childItems << new QStandardItem("激活记录");  // 第0列
        childItems << new QStandardItem(activationDialog->getActivationCode());  // 列1
        childItems << new QStandardItem(activationDialog->getProjectNumber());   // 列2
        childItems << new QStandardItem(activationDialog->getChassisNumber());   // 列3
        
        // 填充剩余列（4-8）为空
        for (int i = 4; i < 9; ++i) {
            childItems << new QStandardItem("");
        }

        // 添加子项到主行
        parentItem->appendRow(childItems);
        qDebug() << "已添加子项，父项现在有" << parentItem->rowCount() << "个子行";

        // 3. 更新剩余激活次数
        int remaining = serialModel->item(index.row(), 2)->text().toInt() - 1;
        serialModel->item(index.row(), 2)->setText(QString::number(remaining));
        
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE serial_numbers SET remaining_activations = ? WHERE serial_number = ?");
        updateQuery.addBindValue(remaining);
        updateQuery.addBindValue(serialNumber);
        
        if (!updateQuery.exec()) {
            QSqlDatabase::database().rollback();
            QMessageBox::critical(this, "错误", "更新激活次数失败: " + updateQuery.lastError().text());
            qDebug() << "更新激活次数失败:" << updateQuery.lastError().text();
            delete activationDialog;
            return;
        }

        // 提交事务
        if (!QSqlDatabase::database().commit()) {
            QMessageBox::critical(this, "错误", "提交事务失败");
            qDebug() << "提交事务失败";
            delete activationDialog;
            return;
        }

        // 刷新视图
        serialTableView->expand(index);
        serialTableView->viewport()->update();
        serialModel->layoutChanged();  // 强制刷新模型
        
        qDebug() << "激活信息添加成功，剩余激活次数:" << remaining;
    }
    
    delete activationDialog;
    // 加载数据
    loadSerialNumbers();
    loadActivationInfo();
}

void MainWindow::deleteSerialNumber()
{
    if (!verifyPassword()) {
        return;
    }

    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid() || index.parent().isValid()) return; // 确保是主行

    QString serialNumber = serialModel->item(index.row(), 0)->text();

    // 从数据库删除主行和所有关联的子行
    QSqlDatabase::database().transaction();
    
    QSqlQuery query;
    query.prepare("DELETE FROM activation_info WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    if (!query.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "错误", "删除关联激活信息失败: " + query.lastError().text());
        return;
    }
    
    query.prepare("DELETE FROM serial_numbers WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    if (!query.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "错误", "删除序列号失败: " + query.lastError().text());
        return;
    }
    
    QSqlDatabase::database().commit();

    // 更新UI
    serialModel->removeRow(index.row());
}

void MainWindow::downloadLicense()
{
    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) return;

    QString serialNumber = serialModel->item(index.row(), 0)->text();
    
    QSqlQuery query;
    query.prepare("SELECT license_file FROM serial_numbers WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    
    if (query.exec() && query.next()) {
        QByteArray fileData = query.value(0).toByteArray();
        if (fileData.isEmpty()) {
            QMessageBox::information(this, "提示", "没有LICENSE文件");
            return;
        }
        
        QString savePath = QFileDialog::getSaveFileName(this, "保存LICENSE文件", 
                                                      serialNumber + ".license", "License Files (*.license)");
        if (!savePath.isEmpty()) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
                QMessageBox::information(this, "成功", "LICENSE文件已保存");
            } else {
                QMessageBox::critical(this, "错误", "无法保存文件");
            }
        }
    }
}

void MainWindow::downloadKyinfo()
{
    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) return;

    QString serialNumber = serialModel->item(index.row(), 0)->text();
    
    QSqlQuery query;
    query.prepare("SELECT kyinfo_file FROM serial_numbers WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    
    if (query.exec() && query.next()) {
        QByteArray fileData = query.value(0).toByteArray();
        if (fileData.isEmpty()) {
            QMessageBox::information(this, "提示", "没有.kyinfo文件");
            return;
        }
        
        QString savePath = QFileDialog::getSaveFileName(this, "保存.kyinfo文件", 
                                                      serialNumber + ".kyinfo", "Kyinfo Files (*.kyinfo)");
        if (!savePath.isEmpty()) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
                QMessageBox::information(this, "成功", ".kyinfo文件已保存");
            } else {
                QMessageBox::critical(this, "错误", "无法保存文件");
            }
        }
    }
}

bool MainWindow::verifyPassword()
{
    bool ok;
    QString password = QInputDialog::getText(this, "密码验证", "请输入密码:", 
                                           QLineEdit::Password, "", &ok);
    
    if (ok && password == "000000") {
        return true;
    } else if (ok) {
        QMessageBox::warning(this, "错误", "密码错误!");
    }
    
    return false;
}

void MainWindow::updateSerialNumberInDatabase(const QModelIndex &index)
{
    QString serialNumber = serialModel->item(index.row(), 0)->text();
    QString columnName;
    
    switch (index.column()) {
    case 0: columnName = "serial_number"; break;
    case 1: columnName = "total_activations"; break;
    case 2: columnName = "remaining_activations"; break;
    case 3: columnName = "platform"; break;
    case 4: columnName = "verification_code"; break;
    case 5: columnName = "license_file"; break;
    case 6: columnName = "kyinfo_file"; break;
    case 7: columnName = "bind_wechat"; break;
    case 8: columnName = "bind_person"; break;
    default: return;
    }
    
    QSqlQuery query;
    query.prepare(QString("UPDATE serial_numbers SET %1 = ? WHERE serial_number = ?").arg(columnName));
    query.addBindValue(index.data().toString());
    query.addBindValue(serialNumber);
    query.exec();
}

void MainWindow::updateActivationInfoInDatabase(const QString &serialNumber, const QModelIndex &index)
{
    int row = index.row();
    QString activationCode = activationModels[serialNumber]->item(row, 0)->text();
    
    QString columnName;
    switch (index.column()) {
    case 0: columnName = "activation_code"; break;
    case 1: columnName = "project_number"; break;
    case 2: columnName = "chassis_number"; break;
    default: return;
    }
    
    QSqlQuery query;
    query.prepare(QString("UPDATE activation_info SET %1 = ? WHERE serial_number = ? AND activation_code = ?").arg(columnName));
    query.addBindValue(index.data().toString());
    query.addBindValue(serialNumber);
    query.addBindValue(activationCode);
    query.exec();
}

CSVData MainWindow::parseCSVFile(const QString &filePath) {
    CSVData data;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开CSV文件:" << file.errorString();
        return data;
    }

    QTextStream in(&file);
    bool isHeader = true;
    bool isMainData = false;
    bool isActivationData = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.isEmpty()) continue;
        
        if (line.contains("激活数据表")) {
            isMainData = true;
            continue;
        }
        
        if (line.contains("注册码,激活码")) {
            isHeader = false;
            isActivationData = true;
            continue;
        }
        
        if (isMainData && !isActivationData) {
            // 处理主数据行，需要更智能的解析方式
            if (line.contains("服务序列号")) {
                // 示例行: "服务序列号,63261116,授权总数,6,"
                QStringList parts = line.split(",");
                for (int i = 0; i < parts.size(); i++) {
                    if (parts[i].contains("服务序列号") && (i+1) < parts.size()) {
                        data.serialNumber = parts[i+1].trimmed();
                    }
                    if (parts[i].contains("授权总数") && (i+1) < parts.size()) {
                        data.totalActivations = parts[i+1].toInt();
                    }
                }
            }
            else if (line.contains("可分配/可取消")) {
                // 示例行: "激活方式,扫码,可分配/可取消,2/0,"
                QStringList parts = line.split(",");
                for (int i = 0; i < parts.size(); i++) {
                    if (parts[i].contains("可分配/可取消") && (i+1) < parts.size()) {
                        QStringList allocParts = parts[i+1].split("/");
                        if (allocParts.size() >= 1) {
                            data.remainingActivations = allocParts[0].toInt();
                            qDebug() << "可分配数量:" << data.remainingActivations;
                        }
                    }
                }
            }
        }
        
        if (isActivationData && !isHeader) {
            // 处理激活码数据行
            QStringList parts = line.split(",");
            if (parts.size() >= 2) {
                QString regCode = parts[0].trimmed();
                QString actCode = parts[1].trimmed();
                if (!regCode.isEmpty() && !actCode.isEmpty()) {
                    data.activationCodes.append(qMakePair(regCode, actCode));
                }
            }
        }
    }
    
    file.close();
    
    qDebug() << "解析结果:";
    qDebug() << "服务序列号:" << data.serialNumber;
    qDebug() << "授权总数:" << data.totalActivations;
    qDebug() << "可分配数量:" << data.remainingActivations;
    qDebug() << "激活码数量:" << data.activationCodes.size();
    
    return data;
}

#if 0
CSVData MainWindow::parseCSVFile(const QString &filePath) {
    CSVData data;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开CSV文件:" << file.errorString();
        return data;
    }

    QTextStream in(&file);
    bool isHeader = true;
    bool isMainData = false;
    bool isActivationData = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.isEmpty()) continue;
        
        if (line.contains("激活数据表")) {
            isMainData = true;
            continue;
        }
        
        if (line.contains("注册码,激活码")) {
            isHeader = false;
            isActivationData = true;
            continue;
        }
        
        if (isMainData && !isActivationData) {
            QStringList parts = line.split(",");
            if (parts.size() >= 4) {
                if (parts[0].contains("服务序列号")) {
                    data.serialNumber = parts[1].trimmed();
                } else if (parts[0].contains("授权总数")) {
                    data.totalActivations = parts[1].toInt();
                } else if (parts[0].contains("可分配/可取消")) {
                    QStringList allocParts = parts[1].split("/");
                    if (allocParts.size() >= 1) {
                        data.remainingActivations = allocParts[0].toInt();
                        qDebug()<<data.remainingActivations;
                    }
                }
            }
        }
        
        if (isActivationData && !isHeader) {
            QStringList parts = line.split(",");
            if (parts.size() >= 3) {
                QString regCode = parts[0].trimmed();
                QString actCode = parts[1].trimmed();
                if (!regCode.isEmpty() && !actCode.isEmpty()) {
                    data.activationCodes.append(qMakePair(regCode, actCode));
                }
            }
        }
    }
    
    file.close();
    return data;
}
#endif
bool MainWindow::addDataToSystem(const CSVData &data)
{
    // 检查序列号是否已存在
    if (isSerialNumberExists(data.serialNumber)) {
        QMessageBox::warning(this, "警告", 
            QString("序列号 %1 已存在，跳过导入").arg(data.serialNumber));
        return false;
    }
    
    // 开始事务
    QSqlDatabase::database().transaction();
    
    try {
        // 1. 插入主行数据到数据库
        QSqlQuery mainQuery;
        mainQuery.prepare(
            "INSERT INTO serial_numbers "
            "(serial_number, total_activations, remaining_activations, "
            "platform, bind_wechat, bind_person) "
            "VALUES (?, ?, ?, '?', '是', 'Excel导入')");
        
        mainQuery.addBindValue(data.serialNumber);
        mainQuery.addBindValue(data.totalActivations);
        mainQuery.addBindValue(data.remainingActivations);
        
        if (!mainQuery.exec()) {
            throw std::runtime_error(
                QString("插入序列号失败: %1").arg(mainQuery.lastError().text()).toStdString());
        }
        
        // 2. 插入激活码到数据库
        for (const auto &codePair : data.activationCodes) {
            QSqlQuery codeQuery;
            codeQuery.prepare(
                "INSERT INTO activation_info "
                "(serial_number, activation_code) "
                "VALUES (?, ?)");
            codeQuery.addBindValue(data.serialNumber);
            codeQuery.addBindValue(codePair.second); // 使用激活码
            
            if (!codeQuery.exec()) {
                qDebug() << "激活码插入失败:" << codeQuery.lastError();
                // 继续插入其他激活码
            }
        }
        
        // 3. 更新UI
        QList<QStandardItem*> mainRow;
        mainRow << new QStandardItem(data.serialNumber);
        mainRow << new QStandardItem(QString::number(data.totalActivations));
        mainRow << new QStandardItem(QString::number(data.remainingActivations));
        mainRow << new QStandardItem("");
        mainRow << new QStandardItem(""); // 验证码
        mainRow << new QStandardItem("无"); // LICENSE
        mainRow << new QStandardItem("无"); // .kyinfo
        mainRow << new QStandardItem("是"); // 绑定微信
        mainRow << new QStandardItem("Excel导入"); // 绑定人
        
        QStandardItem *parentItem = mainRow.first();
        serialModel->appendRow(mainRow);
        
        // 添加子行（激活码）
        for (const auto &codePair : data.activationCodes) {
            QList<QStandardItem*> childRow;
            // 前9列留空（与主行对应）
            for (int i = 0; i < 9; ++i) {
                childRow << new QStandardItem("");
            }
            // 激活码在第9列
            childRow << new QStandardItem(codePair.second);
            parentItem->appendRow(childRow);
        }
        
        // 展开显示
        serialTableView->expand(parentItem->index());
        
        // 提交事务
        if (!QSqlDatabase::database().commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
        
    } catch (const std::exception &e) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "错误", QString::fromStdString(e.what()));
        return false;
    }
}

void MainWindow::importFromCSV() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择CSV文件", "", "CSV文件 (*.csv)");
    
    if (filePath.isEmpty()) return;
    
    // 解析CSV文件
    CSVData data = parseCSVFile(filePath);
    
    if (data.serialNumber.isEmpty()) {
        QMessageBox::warning(this, "警告", "CSV文件格式不正确或没有有效数据");
        return;
    }
    
    // 添加到系统
    if (addDataToSystem(data)) {
        QMessageBox::information(this, "成功", 
            QString("成功导入序列号 %1\n包含 %2 个激活码")
                .arg(data.serialNumber)
                .arg(data.activationCodes.size()));
    }
}

bool MainWindow::isSerialNumberExists(const QString &serialNumber)
{
    // 检查界面中是否存在
    for (int i = 0; i < serialModel->rowCount(); ++i) {
        if (serialModel->item(i, 0)->text() == serialNumber) {
            return true;
        }
    }
    
    // 检查数据库中是否存在
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM serial_numbers WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}
