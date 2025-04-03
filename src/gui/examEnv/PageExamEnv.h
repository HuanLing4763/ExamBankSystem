#pragma once

#include <QWidget>
#include "ui_PageExamEnv.h"

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
    // @param[in] pair 环境配置数据（名称+图标）
    void updateCurrentEnvLabel(const QPair<QString, QIcon>& pair);

private slots:
    // @brief 切换考试环境
    void onSwitchButtonClicked();
};

