#include <QToolButton>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "PageSpecial.h"
#include "env/ExamEnvManager.h"

PageSpecial::PageSpecial(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    // 初始化题型数据
    auto questionTypes = ExamEnvManager::getInstance().getQuestionTypes();

    /* 网格布局配置：
     * - 列间距为0，边距清零
     * - 首尾列自动扩展撑满空间
     * - 中间列固定按钮间距
     */
    ui.gridLayout->setHorizontalSpacing(0);
    ui.gridLayout->setContentsMargins(0, 0, 0, 0);
    ui.gridLayout->setColumnStretch(0, 1);
    ui.gridLayout->setColumnStretch(questionTypes.size() + 1, 1);

    // 动态生成题型按钮
    int index = 0;
    for (const auto& questionType : questionTypes)
    {
        // 创建按钮容器
        QWidget* container = new QWidget(this);
        container->setFixedSize(160, 200);

        // 创建按钮布局
        QVBoxLayout* vLayout = new QVBoxLayout(container);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(4);

        // 创建圆形图标按钮
        QToolButton* button = new QToolButton(container);
        button->setIcon(questionType.second);
        button->setIconSize(QSize(128, 128));
        button->setFixedSize(128, 128);

        // 设置按钮样式
        button->setStyleSheet(
            "QToolButton {"
            "   border-radius: 64px;"
            "   border: 2px solid #0078d4;"
            "   background: white;"
            "}"
            "QToolButton:hover {"
            "   border-color: #0078d4;"
            "   background-color: #f0f6fc;"
            "}"
            "QToolButton:pressed {"
            "   background-color: #cce4f7;"
            "}");

        // 存储题型标识并连接信号
        button->setProperty("questionType", questionType.first);
        connect(button, &QToolButton::clicked, this, &PageSpecial::onButtonClicked);

        // 创建题型名称标签
        QLabel* label = new QLabel(questionType.first, container);
        label->setAlignment(Qt::AlignCenter);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label->setFixedHeight(40);
        label->setFont(QFont("Microsoft YaHei", 18));

        // 将组件加入布局
        vLayout->addWidget(button, 0, Qt::AlignHCenter);
        vLayout->addWidget(label);

        // 将容器加入网格布局
        ui.gridLayout->addWidget(container, 0, ++index, Qt::AlignCenter);
    }
}

PageSpecial::~PageSpecial()
{}

void PageSpecial::onButtonClicked()
{
    // TODO: 跳转到对应题型练习页面
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (button) {
        QString type = button->property("questionType").toString();
        qDebug() << "Clicked on:" << type;
    }
}