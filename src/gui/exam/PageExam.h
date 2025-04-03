#pragma once

#include <QWidget>
#include "virtualbox/VirtualBoxController.h"
#include "ui_PageExam.h"

/**
 * @brief 模拟考试页面，负责虚拟考试环境启动控制
 * @details 主要功能包括参数配置、虚拟机启动
 */
class PageExam : public QWidget
{
    Q_OBJECT

public:
    PageExam(QWidget* parent = nullptr);
    ~PageExam();

private:
    Ui::ExamPage ui;

    // @brief 获取当前考试参数（临时测试数据）
    QJsonArray getAppParams();

    // @brief 将考试参数写入共享文件夹
    // @param[in] params 需要写入的JSON参数数组
    // @return 是否写入成功
    bool writeParamsToSharedFolder(const QJsonArray& params);

private slots:
    // @brief 考试按钮点击事件
    void onBtnExamClicked();
};
