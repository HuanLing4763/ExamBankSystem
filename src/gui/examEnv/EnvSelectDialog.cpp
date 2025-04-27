#include <QListWidgetItem>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "EnvSelectDialog.h"
#include "DownloadProgressDialog.h"
#include "virtualbox/VirtualBoxController.h"

#define VIRTUALBOX_IMAGES_DIR "E:/Project/CPP/ExamBankSystem/images"

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

        QPushButton* actionButton = new QPushButton(vmExists ? "确认" : "下载", itemWidget);
        actionButton->setProperty("env_id", env.id);

        itemLayout->addWidget(iconLabel);
        itemLayout->addWidget(nameLabel);
        itemLayout->addStretch();
        itemLayout->addWidget(actionButton);

        ui.listEnv->setItemWidget(item, itemWidget);

        connect(actionButton, &QPushButton::clicked, [this, env, actionButton]() {
            if (actionButton->text() == "下载") {
                DownloadProgressDialog* progressDlg = new DownloadProgressDialog(this);
                VirtualBoxController* controller = VirtualBoxController::getInstance();

                connect(controller, &VirtualBoxController::downloadProgress,
                    progressDlg, &DownloadProgressDialog::updateDownloadProgress);
                connect(controller, &VirtualBoxController::downloadFinished,
                    [progressDlg, actionButton, env, controller](bool success) {
                        if (success) {
                            progressDlg->startImport(env.vmName);
                            QString ovaFilePath = QString("%1/%2.ova").arg(VIRTUALBOX_IMAGES_DIR).arg(env.vmName);
                            connect(controller, &VirtualBoxController::vmImportProgress,
                                progressDlg, &DownloadProgressDialog::updateImportProgress);
                            connect(controller, &VirtualBoxController::importFinished,
                                [progressDlg, actionButton, env](bool importSuccess) {
                                    if (importSuccess && VirtualBoxController::getInstance()->isVMExists(env.vmName)) {
                                        actionButton->setText("确认");
                                    }
                                    else {
                                        // 导入失败，更新按钮状态和文本
                                        actionButton->setEnabled(true);
                                        actionButton->setText("下载");
                                    }
                                    progressDlg->finishImport(importSuccess);
                                });
                            controller->createVM(ovaFilePath);
                        }
                        else {
                            progressDlg->finishDownload(false);
                            actionButton->setEnabled(true);
                            actionButton->setText("下载");
                        }
                    });

                controller->downloadVMImage(env.vmName);
                actionButton->setEnabled(false);
                actionButton->setText("下载中...");
                progressDlg->show();
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