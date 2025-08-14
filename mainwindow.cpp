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
    bindPersonEdit->setEnabled(false);
    
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
                                          "验证码", "LICENSE", ".kyinfo", "绑定微信", "绑定人"});
    serialTableView->setModel(serialModel);
//    serialTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    serialTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    serialTableLayout->addWidget(serialTableView);
    mainLayout->addWidget(serialTableGroup);
    
    // 连接信号槽
    connect(serialTableView, &QTreeView::customContextMenuRequested, this, &MainWindow::showSerialContextMenu);
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
            // 创建子项（9列，与主模型一致）
            QList<QStandardItem*> childItems;
            childItems << new QStandardItem(activationCode);    // 列0
            childItems << new QStandardItem(projectNumber);     // 列1
            childItems << new QStandardItem(chassisNumber);     // 列2
            for (int i = 3; i < 9; ++i) {
                childItems << new QStandardItem("");  // 填充剩余列
            }
            // 添加到主行下
            parentItem->appendRow(childItems);
        }
    }
    #if 0
    activationModels.clear();

    QSqlQuery query("SELECT * FROM activation_info");
    while (query.next()) {
        QString serialNumber = query.value("serial_number").toString();
        
        if (!activationModels.contains(serialNumber)) {
            QStandardItemModel *model = new QStandardItemModel(this);
            model->setHorizontalHeaderLabels({"激活码", "项目号", "机箱序列号"});
            activationModels[serialNumber] = model;
        }
        
        QList<QStandardItem*> items;
        items << new QStandardItem(query.value("activation_code").toString());
        items << new QStandardItem(query.value("project_number").toString());
        items << new QStandardItem(query.value("chassis_number").toString());
        
        activationModels[serialNumber]->appendRow(items);
    }
    #endif
}

void MainWindow::addSerialNumber()
{
    QString serialNumber = serialNumberEdit->text().trimmed();
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

    QAction *modifyAction = new QAction("修改", this);
    connect(modifyAction, &QAction::triggered, this, &MainWindow::modifySerialNumber);
    contextMenu.addAction(modifyAction);

    QAction *addAction = new QAction("增加激活信息", this);
    connect(addAction, &QAction::triggered, this, &MainWindow::addActivationInfo);
    contextMenu.addAction(addAction);

    QAction *deleteAction = new QAction("删除", this);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSerialNumber);
    contextMenu.addAction(deleteAction);

    QAction *downloadLicenseAction = new QAction("下载LICENSE", this);
    connect(downloadLicenseAction, &QAction::triggered, this, &MainWindow::downloadLicense);
    contextMenu.addAction(downloadLicenseAction);

    QAction *downloadKyinfoAction = new QAction("下载.kyinfo", this);
    connect(downloadKyinfoAction, &QAction::triggered, this, &MainWindow::downloadKyinfo);
    contextMenu.addAction(downloadKyinfoAction);

    contextMenu.exec(serialTableView->viewport()->mapToGlobal(pos));
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
    if (!index.isValid()) return;

    // 确保选中的是顶层主行（不是子项）
    if (index.parent().isValid()) {
        QMessageBox::warning(this, "提示", "请选择主行添加激活信息");
        return;
    }

    QStandardItem *parentItem = serialModel->itemFromIndex(index);
    if (!parentItem) return;

    QString serialNumber = serialModel->item(index.row(), 0)->text();
    activationDialog = new ActivationDialog(serialNumber, this);

    if (activationDialog->exec() == QDialog::Accepted) {
        // 1. 数据库插入（保持不变）
        QSqlQuery query;
        query.prepare("INSERT INTO activation_info (serial_number, activation_code, project_number, chassis_number) "
                    "VALUES (?, ?, ?, ?)");
        query.addBindValue(serialNumber);
        query.addBindValue(activationDialog->getActivationCode());
        query.addBindValue(activationDialog->getProjectNumber());
        query.addBindValue(activationDialog->getChassisNumber());
        
        if (!query.exec()) {
            QMessageBox::critical(this, "错误", "添加激活信息失败: " + query.lastError().text());
            return;
        }

        // 2. 添加子项（关键修改：列数与主模型保持一致，共9列）
        QList<QStandardItem*> childItems;
        // 前3列：激活信息（对应主模型的列0-2，内容自定义）
        childItems << new QStandardItem(activationDialog->getActivationCode());  // 列0
        childItems << new QStandardItem(activationDialog->getProjectNumber());   // 列1
        childItems << new QStandardItem(activationDialog->getChassisNumber());   // 列2
        // 剩余6列：留空（匹配主模型的9列，列3-8）
        for (int i = 3; i < 9; ++i) {
            childItems << new QStandardItem("");  // 空值填充，确保列数一致
        }

        // 添加子项到主行
        parentItem->appendRow(childItems);
        // 自动展开主行
        serialTableView->expand(index);

        // 3. 更新剩余激活次数（保持不变）
        int remaining = serialModel->item(index.row(), 2)->text().toInt() - 1;
        serialModel->item(index.row(), 2)->setText(QString::number(remaining));
        
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE serial_numbers SET remaining_activations = ? WHERE serial_number = ?");
        updateQuery.addBindValue(remaining);
        updateQuery.addBindValue(serialNumber);
        updateQuery.exec();
    }
    delete activationDialog;
#if 0
    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) return;

    QString serialNumber = serialModel->item(index.row(), 0)->text();
    activationDialog = new ActivationDialog(serialNumber, this);
    
    if (activationDialog->exec() == QDialog::Accepted) {
        // 插入数据库
        QSqlQuery query;
        query.prepare("INSERT INTO activation_info (serial_number, activation_code, project_number, chassis_number) "
                     "VALUES (?, ?, ?, ?)");
        query.addBindValue(serialNumber);
        query.addBindValue(activationDialog->getActivationCode());
        query.addBindValue(activationDialog->getProjectNumber());
        query.addBindValue(activationDialog->getChassisNumber());
        
        if (!query.exec()) {
            QMessageBox::critical(this, "错误", "添加激活信息失败: " + query.lastError().text());
            return;
        }

        // 更新UI
        if (!activationModels.contains(serialNumber)) {
            QStandardItemModel *model = new QStandardItemModel(this);
            model->setHorizontalHeaderLabels({"激活码", "项目号", "机箱序列号"});
            activationModels[serialNumber] = model;
        }
        
        QList<QStandardItem*> items;
        items << new QStandardItem(activationDialog->getActivationCode());
        items << new QStandardItem(activationDialog->getProjectNumber());
        items << new QStandardItem(activationDialog->getChassisNumber());
        
        activationModels[serialNumber]->appendRow(items);

        // 更新剩余激活次数
        int remaining = serialModel->item(index.row(), 2)->text().toInt() - 1;
        serialModel->item(index.row(), 2)->setText(QString::number(remaining));
        
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE serial_numbers SET remaining_activations = ? WHERE serial_number = ?");
        updateQuery.addBindValue(remaining);
        updateQuery.addBindValue(serialNumber);
        updateQuery.exec();
    }
    delete activationDialog;
#endif
}

void MainWindow::deleteSerialNumber()
{
    if (!verifyPassword()) {
        return;
    }

    QModelIndex index = serialTableView->currentIndex();
    if (!index.isValid()) return;

    QString serialNumber = serialModel->item(index.row(), 0)->text();

    // 从数据库删除
    QSqlQuery query;
    query.prepare("DELETE FROM serial_numbers WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    
    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "删除序列号失败: " + query.lastError().text());
        return;
    }

    // 删除关联的激活信息
    query.prepare("DELETE FROM activation_info WHERE serial_number = ?");
    query.addBindValue(serialNumber);
    query.exec();

    // 更新UI
    serialModel->removeRow(index.row());
    activationModels.remove(serialNumber);
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
