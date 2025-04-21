#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QBuffer>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include "PageExamEnv.h"
#include "EnvSelectDialog.h"

PageExamEnv::PageExamEnv(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.switchButton, &QPushButton::clicked, this, &PageExamEnv::onSwitchButtonClicked);

    // 初始化当前环境
    updateCurrentEnvLabel();
}

PageExamEnv::~PageExamEnv()
{}

void PageExamEnv::updateCurrentEnvLabel()
{
    const auto& env = ExamEnvManager::getInstance().currentEnvInfo();
    ui.currentEnvIconLabel->setPixmap(env.icon.pixmap(120, 120));
    ui.currentEnvTextLabel->setText(env.name);
}

void PageExamEnv::onEnvSelected(int envId)
{
    auto& envManager = ExamEnvManager::getInstance();
    auto it = std::find_if(envManager.envList().begin(), envManager.envList().end(),
        [envId](const auto& env) { return env.id == envId; });

    if (it != envManager.envList().end()) {
        envManager.setCurrentEnv(*it);
        updateCurrentEnvLabel();
    }
}

void PageExamEnv::onSwitchButtonClicked()
{
    EnvSelectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ExamEnvManager::getInstance().setCurrentEnv(dialog.selectedEnv());
        updateCurrentEnvLabel();
    }
}
