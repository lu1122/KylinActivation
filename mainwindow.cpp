#include "mainwindow.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QMenu>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 初始化UI
    setupUI();

    // 初始化数据库
    initDatabase();

    // 设置表格视图
    setupTableView();

    // 加载数据
    loadData();

    // 设置窗口属性
    setWindowTitle("银河麒麟激活码管理平台");
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    db.close();
}

void MainWindow::setupUI()
{
    // 创建中央部件和主布局
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // 创建输入区域
    inputGroup = new QGroupBox("激活码信息", centralWidget);
    inputLayout = new QGridLayout(inputGroup);

    // 序列码
    serialLabel = new QLabel("序列码:", inputGroup);
    serialEdit = new QLineEdit(inputGroup);
    inputLayout->addWidget(serialLabel, 0, 0);
    inputLayout->addWidget(serialEdit, 0, 1);

    // 激活码
    codeLabel = new QLabel("激活码:", inputGroup);
    codeEdit = new QLineEdit(inputGroup);
    inputLayout->addWidget(codeLabel, 1, 0);
    inputLayout->addWidget(codeEdit, 1, 1);

    // 项目号
    projectLabel = new QLabel("项目号:", inputGroup);
    projectEdit = new QLineEdit(inputGroup);
    inputLayout->addWidget(projectLabel, 2, 0);
    inputLayout->addWidget(projectEdit, 2, 1);

    // 机箱序列号
    chassisLabel = new QLabel("机箱序列号:", inputGroup);
    chassisEdit = new QLineEdit(inputGroup);
    inputLayout->addWidget(chassisLabel, 3, 0);
    inputLayout->addWidget(chassisEdit, 3, 1);

    // LICENSE文件
    licenseLabel = new QLabel("LICENSE文件:", inputGroup);
    licenseEdit = new QLineEdit(inputGroup);
    browseLicenseButton = new QPushButton("浏览...", inputGroup);
    inputLayout->addWidget(licenseLabel, 4, 0);
    inputLayout->addWidget(licenseEdit, 4, 1);
    inputLayout->addWidget(browseLicenseButton, 4, 2);
    connect(browseLicenseButton, &QPushButton::clicked, this, &MainWindow::onBrowseLicenseClicked);

    // KYINFO文件
    kyinfoLabel = new QLabel("KYINFO文件:", inputGroup);
    kyinfoEdit = new QLineEdit(inputGroup);
    browseKyinfoButton = new QPushButton("浏览...", inputGroup);
    inputLayout->addWidget(kyinfoLabel, 5, 0);
    inputLayout->addWidget(kyinfoEdit, 5, 1);
    inputLayout->addWidget(browseKyinfoButton, 5, 2);
    connect(browseKyinfoButton, &QPushButton::clicked, this, &MainWindow::onBrowseKyinfoClicked);

    inputGroup->setLayout(inputLayout);
    mainLayout->addWidget(inputGroup);

    // 创建按钮区域
    buttonLayout = new QHBoxLayout();

    addButton = new QPushButton("添加", centralWidget);
    deleteButton = new QPushButton("删除", centralWidget);
    exportButton = new QPushButton("导出", centralWidget);
    importButton = new QPushButton("导入", centralWidget);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(importButton);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportButtonClicked);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::onImportButtonClicked);

    mainLayout->addLayout(buttonLayout);

    // 创建表格视图
    tableView = new QTableView(centralWidget);
    mainLayout->addWidget(tableView);
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
connect(tableView, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
    QModelIndex index = tableView->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();
    QString licenseName = model->data(model->index(row, model->fieldIndex("license_file_name"))).toString();
    QString kyinfoName = model->data(model->index(row, model->fieldIndex("kyinfo_file_name"))).toString();

    QMenu menu;
    QAction *downloadLicense = menu.addAction("下载LICENSE文件: " + licenseName);
    QAction *downloadKyinfo = menu.addAction("下载KYINFO文件: " + kyinfoName);

    QAction *selected = menu.exec(tableView->viewport()->mapToGlobal(pos));
    if (selected == downloadLicense) {
        QString savePath = QFileDialog::getSaveFileName(this, "保存LICENSE文件", licenseName);
        if (!savePath.isEmpty()) {
            QByteArray fileData = model->data(model->index(row, model->fieldIndex("license_file")), Qt::EditRole).toByteArray();
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
                QMessageBox::information(this, "成功", "LICENSE文件保存成功！");
            } else {
                QMessageBox::critical(this, "错误", "无法保存文件: " + file.errorString());
            }
        }
    } else if (selected == downloadKyinfo) {
        QString savePath = QFileDialog::getSaveFileName(this, "保存KYINFO文件", kyinfoName);
        if (!savePath.isEmpty()) {
            QByteArray fileData = model->data(model->index(row, model->fieldIndex("kyinfo_file")), Qt::EditRole).toByteArray();
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
                QMessageBox::information(this, "成功", "KYINFO文件保存成功！");
            } else {
                QMessageBox::critical(this, "错误", "无法保存文件: " + file.errorString());
            }
        }
    }
});
}

void MainWindow::initDatabase()
{
    // 使用SQLite数据库
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("kylin_activation.db");

    if (!db.open()) {
        QMessageBox::critical(this, "错误", "无法打开数据库: " + db.lastError().text());
        return;
    }

    // 创建表（如果不存在）
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS activation_codes ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "serial_code TEXT NOT NULL, "
           "activation_code TEXT NOT NULL, "
           "project_number TEXT, "
           "chassis_serial TEXT, "
           "license_file BLOB, "  // 改为BLOB类型存储文件内容
           "kyinfo_file BLOB, "   // 改为BLOB类型存储文件内容
           "license_file_name TEXT, "  // 新增原始文件名
           "kyinfo_file_name TEXT, "   // 新增原始文件名
           "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
}

void MainWindow::setupTableView()
{
    model = new QSqlTableModel(this, db);
    model->setTable("activation_codes");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    // 设置列标题 - 将时间设为第一列
    model->setHeaderData(9, Qt::Horizontal, "创建时间");  // 时间列设为第一列显示
    model->setHeaderData(1, Qt::Horizontal, "序列码");
    model->setHeaderData(2, Qt::Horizontal, "激活码");
    model->setHeaderData(3, Qt::Horizontal, "项目号");
    model->setHeaderData(4, Qt::Horizontal, "机箱序列号");
    model->setHeaderData(7, Qt::Horizontal, "LICENSE文件名");
    model->setHeaderData(8, Qt::Horizontal, "KYINFO文件名");

    tableView->setModel(model);

    // 设置列显示顺序 - 将时间列移动到第一位置
    tableView->setColumnHidden(0, true);  // 隐藏ID列
    tableView->setColumnHidden(5, true);  // 隐藏LICENSE文件内容
    tableView->setColumnHidden(6, true);  // 隐藏KYINFO文件内容

    // 重新排列列顺序: 时间,序列码,激活码,项目号,机箱序列号,LICENSE文件名,KYINFO文件名
    tableView->horizontalHeader()->moveSection(9, 0);  // 将时间列移动到第一位置

    // 设置时间列显示格式
    model->setHeaderData(0, Qt::Horizontal, "创建时间");
    tableView->setColumnWidth(0, 150);  // 设置时间列宽度

    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MainWindow::loadData()
{
    if (!model->select()) {
        QMessageBox::critical(this, "错误", "加载数据失败: " + model->lastError().text());
    }
}

bool MainWindow::addActivationCode(const QString &serial, const QString &code,
                                 const QString &project, const QString &chassis,
                                 const QString &licenseFilePath, const QString &kyinfoFilePath)
{
    // 读取LICENSE文件内容
    QFile licenseFile(licenseFilePath);
    if (!licenseFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开LICENSE文件: " + licenseFile.errorString());
        return false;
    }
    QByteArray licenseData = licenseFile.readAll();
    licenseFile.close();

    // 读取KYINFO文件内容
    QFile kyinfoFile(kyinfoFilePath);
    if (!kyinfoFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开KYINFO文件: " + kyinfoFile.errorString());
        return false;
    }
    QByteArray kyinfoData = kyinfoFile.readAll();
    kyinfoFile.close();

    QSqlQuery query;
        query.prepare("INSERT INTO activation_codes "
                     "(serial_code, activation_code, project_number, chassis_serial, "
                     "license_file, kyinfo_file, license_file_name, kyinfo_file_name) "
                     "VALUES "
                     "(:serial, :code, :project, :chassis, "
                     ":license, :kyinfo, :license_name, :kyinfo_name)");

        // 确保这8个参数都正确绑定
        query.bindValue(":serial", serial);
        query.bindValue(":code", code);
        query.bindValue(":project", project);
        query.bindValue(":chassis", chassis);
        query.bindValue(":license", licenseData);
        query.bindValue(":kyinfo", kyinfoData);
        query.bindValue(":license_name", QFileInfo(licenseFilePath).fileName());
        query.bindValue(":kyinfo_name", QFileInfo(kyinfoFilePath).fileName());

        if (!query.exec()) {
            QMessageBox::critical(this, "错误", "添加记录失败: " + query.lastError().text());
            return false;
        }

    return true;
}

void MainWindow::onAddButtonClicked()
{
    QString serial = serialEdit->text().trimmed();
    QString code = codeEdit->text().trimmed();
    QString project = projectEdit->text().trimmed();
    QString chassis = chassisEdit->text().trimmed();
    QString licenseFile = licenseEdit->text();
    QString kyinfoFile = kyinfoEdit->text();

    if (serial.isEmpty() || code.isEmpty()) {
        QMessageBox::warning(this, "警告", "序列码和激活码不能为空！");
        return;
    }

    if (addActivationCode(serial, code, project, chassis, licenseFile, kyinfoFile)) {
        // 清空输入框
        serialEdit->clear();
        codeEdit->clear();
        projectEdit->clear();
        chassisEdit->clear();
        licenseEdit->clear();
        kyinfoEdit->clear();

        // 刷新表格
        loadData();

        QMessageBox::information(this, "成功", "激活码添加成功！");
    }
}

void MainWindow::onDeleteButtonClicked()
{
    QModelIndexList selection = tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要删除的行！");
        return;
    }

    int row = selection.first().row();
    int id = model->data(model->index(row, 0)).toInt();

    QSqlQuery query;
    query.prepare("DELETE FROM activation_codes WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        loadData();
        QMessageBox::information(this, "成功", "记录删除成功！");
    } else {
        QMessageBox::critical(this, "错误", "删除失败: " + query.lastError().text());
    }
}

void MainWindow::onExportButtonClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据", "", "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    // 写入表头
    out << tr("创建时间,序列码,激活码,项目号,机箱序列号,LICENSE文件名,KYINFO文件名\n");

    // 写入数据 - 包含时间列
    QSqlQuery query("SELECT created_at, serial_code, activation_code, project_number, "
                   "chassis_serial, license_file_name, kyinfo_file_name FROM activation_codes");
    while (query.next()) {
        out << "\"" << query.value(0).toString() << "\","
            << "\"" << query.value(1).toString() << "\","
            << "\"" << query.value(2).toString() << "\","
            << "\"" << query.value(3).toString() << "\","
            << "\"" << query.value(4).toString() << "\","
            << "\"" << query.value(5).toString() << "\","
            << "\"" << query.value(6).toString() << "\"\n";
    }

    file.close();
    QMessageBox::information(this, "成功", "数据导出成功！");
}

void MainWindow::onImportButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入数据", "", "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件: " + file.errorString());
        return;
    }

    QTextStream in(&file);
    int importedCount = 0;

    // 跳过表头
    in.readLine();

    db.transaction();

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(',');

        if (fields.size() < 6) continue;

        // 去除字段中的引号
        for (int i = 0; i < fields.size(); ++i) {
            fields[i] = fields[i].trimmed();
            if (fields[i].startsWith('"') && fields[i].endsWith('"')) {
                fields[i] = fields[i].mid(1, fields[i].length() - 2);
            }
        }

        if (addActivationCode(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5])) {
            importedCount++;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::critical(this, "错误", "导入过程中出错，已回滚！");
        return;
    }

    file.close();
    loadData();
    QMessageBox::information(this, "成功", QString("成功导入 %1 条记录！").arg(importedCount));
}

void MainWindow::onBrowseLicenseClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择LICENSE文件");
    if (!fileName.isEmpty()) {
        licenseEdit->setText(fileName);
    }
}

void MainWindow::onBrowseKyinfoClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择KYINFO文件");
    if (!fileName.isEmpty()) {
        kyinfoEdit->setText(fileName);
    }
}
