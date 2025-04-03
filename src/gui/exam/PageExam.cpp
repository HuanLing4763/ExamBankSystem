#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QLabel>
#include <QGuiApplication>
#include <QScreen>
#include <QMovie>
#include <QQuickView>
#include <QQuickWidget>
#include "PageExam.h"
#include "env/ExamEnvManager.h"
#include "virtualbox/LoadingManager.h"


PageExam::PageExam(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.btnEnterExam, &QPushButton::clicked,
        this, &PageExam::onBtnExamClicked);
}

PageExam::~PageExam()
{}

void PageExam::onBtnExamClicked() {
    // 获取虚拟机名称和参数
    QString vmName = ExamEnvManager::getInstance().currentEnv();
    QJsonArray params = getAppParams();
    if (!writeParamsToSharedFolder(params)) {
        QMessageBox::critical(this, "错误", "无法写入参数到共享文件夹");
        return;
    }

    // 获取加载管理器实例
    LoadingManager* loader = LoadingManager::getInstance();

    VirtualBoxController* vboxController = VirtualBoxController::getInstance();

    // 连接虚拟机控制器的信号
    connect(vboxController, &VirtualBoxController::vmStarted, this, [=]() {
        loader->hide();
        qDebug() << "虚拟机启动成功";
        }, Qt::QueuedConnection);

    connect(vboxController, &VirtualBoxController::vmStartFailed, this, [=](const QString& error) {
        loader->hide();
        QMessageBox::critical(this, "错误", error);
        }, Qt::QueuedConnection);

    // 显示加载界面
    loader->show();

    // 异步启动虚拟机
    vboxController->startVMAsync(vmName, "gui");
}


QJsonArray PageExam::getAppParams()
{
    // TODO: 需替换为实际考试参数获取逻辑
    // 这里仅返回示例参数
    QJsonArray params;
    params.append("param1");
    params.append("param2");
    return params;
}

bool PageExam::writeParamsToSharedFolder(const QJsonArray& params)
{
    const QString fullPath = QDir(ExamEnvManager::getInstance().getSharedFolderPath())
        .absoluteFilePath("params.json");

    QFile file(fullPath);

    // 自动创建目录并尝试写入
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const bool success = file.write(QJsonDocument(params).toJson()) != -1;
        file.close();
        return success;
    }

    qDebug() << "保存失败：" << file.errorString()
        << "\n路径：" << QDir::toNativeSeparators(fullPath);
    return false;
}