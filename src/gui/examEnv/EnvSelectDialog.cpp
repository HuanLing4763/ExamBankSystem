#include <QListWidgetItem>
#include "EnvSelectDialog.h"

EnvSelectDialog::EnvSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    QLayout* layout = this->layout();
    if (layout) {
        layout->setSizeConstraint(QLayout::SetNoConstraint);
    }

    ui.listEnv->clear();

    const auto& envList = ExamEnvManager::getInstance().envList();
    const auto& currentEnv = ExamEnvManager::getInstance().currentEnvInfo();
    int selectedIndex = 0;

    for (int i = 0; i < envList.size(); ++i) {
        const auto& env = envList[i];
        QListWidgetItem* item = new QListWidgetItem(env.icon, env.name);
        item->setData(Qt::UserRole, env.id);
        item->setSizeHint(QSize(400, 60));
        ui.listEnv->addItem(item);

        if (env.id == currentEnv.id) {
            selectedIndex = i;
        }
    }

    QListWidgetItem* selectedItem = ui.listEnv->item(selectedIndex);
    ui.listEnv->setCurrentItem(selectedItem);
    onItemClicked(selectedItem);

    // 连接列表项点击信号到 onItemClicked 槽函数
    connect(ui.listEnv, &QListWidget::itemClicked, this, &EnvSelectDialog::onItemClicked);

    // 连接确认按钮的点击信号到 accept 槽
    connect(ui.confirmButton, &QPushButton::clicked, this, &EnvSelectDialog::accept);
}

EnvSelectDialog::~EnvSelectDialog()
{}

void EnvSelectDialog::onItemClicked(QListWidgetItem* item)
{
    int envId = item->data(Qt::UserRole).toInt();
    const auto& envList = ExamEnvManager::getInstance().envList();

    auto it = std::find_if(envList.begin(), envList.end(),
        [envId](const auto& env) { return env.id == envId; });

    qDebug() << "Selected env: " << it->name;
    if (it != envList.end()) {
        m_selectedEnv = *it;
    }
}