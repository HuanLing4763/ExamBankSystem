#include <QMenu>
#include <QMessageBox>
#include <QBuffer>
#include "PageExamEnv.h"
#include "env/ExamEnvManager.h"

PageExamEnv::PageExamEnv(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.switchButton, &QPushButton::clicked, this, &PageExamEnv::onSwitchButtonClicked);
    connect(&ExamEnvManager::getInstance(), &ExamEnvManager::envDetailChanged,
        this, [this](const auto& pair) {
            updateCurrentEnvLabel(pair);  // 环境变化时自动更新显示
        });

    // 初始化当前环境
    auto pair = ExamEnvManager::getInstance().currentEnvPair();
    updateCurrentEnvLabel(pair);
}

PageExamEnv::~PageExamEnv()
{}

void PageExamEnv::updateCurrentEnvLabel(const QPair<QString, QIcon>& pair)
{
    QPixmap pixmap = pair.second.pixmap(QSize(120, 120));
    ui.currentEnvIconLabel->setPixmap(pixmap);
    ui.currentEnvTextLabel->setText(pair.first);
}

void PageExamEnv::onSwitchButtonClicked()
{
    // TODO: 实现环境切换功能
    // 预期行为：
    // 1. 弹出环境选择菜单
    // 2. 调用ExamEnvManager切换环境
    // 3. 通过信号通知其他组件更新
}