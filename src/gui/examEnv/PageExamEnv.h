#pragma once

#include "env/ExamEnvManager.h"
#include "ui_PageExamEnv.h"
#include <QWidget>

/**
 * @brief 考试环境管理页面，负责当前考试环境的展示与切换
 * @details 实时同步考试环境管理器的状态变化，提供可视化环境切换入口
 */
class PageExamEnv : public QWidget
{
    Q_OBJECT
public:
    PageExamEnv(QWidget *parent = nullptr);
    ~PageExamEnv();

private:
    Ui::ExamEnvPage ui;
    // @brief 更新当前环境显示标签
    void updateCurrentEnvLabel();
    // @brief 切换考试环境
    void onSwitchButtonClicked();

private slots:
    // @brief 切换考试环境信号
    void onEnvSelected(int envId);
};

