#include "DownloadProgressDialog.h"
#include "EnvSelectDialog.h"
#include "virtualbox/VirtualBoxController.h"
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QPointer>
#include <QPushButton>
#include <QFileDialog>

EnvSelectDialog::EnvSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.listEnv->clear();

    const auto& envList = ExamEnvManager::getInstance().envList();
    const auto& currentEnv = ExamEnvManager::getInstance().currentEnvInfo();

    for (int i = 0; i < envList.size(); ++i) {
        const auto& env = envList[i];
        bool vmExists = VirtualBoxController::getInstance()->isVMExists(env.vmName);

        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, env.id);
        item->setSizeHint(QSize(400, 60));
        ui.listEnv->addItem(item);

        QWidget* itemWidget = new QWidget(ui.listEnv);
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(10, 5, 10, 5);

        QLabel* iconLabel = new QLabel(itemWidget);
        iconLabel->setPixmap(env.icon.pixmap(32, 32));
        QLabel* nameLabel = new QLabel(env.name, itemWidget);

        QPushButton* actionButton = new QPushButton(vmExists ? "确认" : "导入", itemWidget);
        actionButton->setProperty("env_id", env.id);

        itemLayout->addWidget(iconLabel);
        itemLayout->addWidget(nameLabel);
        itemLayout->addStretch();
        itemLayout->addWidget(actionButton);

        ui.listEnv->setItemWidget(item, itemWidget);

        connect(actionButton, &QPushButton::clicked, [this, env, actionButton]() {
            if (actionButton->text() == "导入") {
                DownloadProgressDialog* progressDlg = new DownloadProgressDialog(this);
                VirtualBoxController* controller = VirtualBoxController::getInstance();

                // 连接导入信号
                connect(controller, &VirtualBoxController::vmImportProgress,
                    progressDlg, &DownloadProgressDialog::updateImportProgress);
                connect(controller, &VirtualBoxController::importFinished,
                    [progressDlg, actionButton](bool importSuccess) {
                        QPointer<DownloadProgressDialog> safeDlgPtr(progressDlg);
                        QPointer<QPushButton> safeButton = actionButton;
                        QMetaObject::invokeMethod(qApp, [=]() {
                            if (safeDlgPtr) {
                                safeDlgPtr->finishImport(importSuccess);
                            }
                            if (safeButton) {
                                safeButton->setText(importSuccess ? "确认" : "导入");
                                safeButton->setEnabled(true);
                            }
                            });
                    });

                // 打开文件选择对话框选择本地 OVA 文件
                QString ovaFilePath = QFileDialog::getOpenFileName(this, "选择本地 OVA 文件", "", "OVA 文件 (*.ova)");
                if (!ovaFilePath.isEmpty()) {
                    controller->createVM(ovaFilePath);
                    actionButton->setEnabled(false);
                    actionButton->setText("导入中...");
                    progressDlg->startImport(env.vmName);
                    progressDlg->show();
                }
            }
            else {
                m_selectedEnv = env;
                accept();
            }
            });

        if (env.id == currentEnv.id) {
            m_selectedEnv = env;
        }
    }
}

EnvSelectDialog::~EnvSelectDialog()
{}