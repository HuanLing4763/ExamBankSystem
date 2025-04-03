#pragma once

#include <QtWidgets/QMainWindow>
#include <QtCore/QPropertyAnimation>
#include <QSystemTrayIcon>
#include <QMenu>
#include "ui_ExamBankSystem.h"
#include "special/PageSpecial.h"
#include "exam/PageExam.h"
#include "examEnv/PageExamEnv.h"

/**
 * @brief 题库管理系统主窗口，负责页面导航和窗口控制
 * @details 管理三个核心功能模块：专项练习、模拟考试、考试环境管理，
 *          处理自定义标题栏拖拽和侧边栏动画效果
 */
class ExamBankSystem : public QMainWindow
{
    Q_OBJECT

public:
    ExamBankSystem(QWidget* parent = nullptr);
    ~ExamBankSystem();

protected:
    // 重写鼠标事件实现窗口拖拽
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    // 事件过滤器处理按钮交互效果
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // @brief 统一处理页面切换按钮点击事件
    void onButtonClicked();

    void onExitApp();    // 退出程序

private:
    Ui::MainWindow ui;

    QSystemTrayIcon* trayIcon;    // 托盘图标
    void createTrayIcon();        // 创建托盘图标

    PageSpecial* specialPage;     // 专项练习页面
    PageExam* examPage;           // 模拟考试页面
    PageExamEnv* examEnvPage;     // 考试环境管理页面

    QPoint windowDragPosition;    // 窗口拖拽起始位置
    bool isDraggingTitleBar;      // 标题栏拖拽状态标识

    QPropertyAnimation* expandAnimation;   // 侧边栏展开动画
    QPropertyAnimation* collapseAnimation; // 侧边栏收起动画

    // @brief 处理侧边栏按钮的悬停展开/离开收起动画
    // @param[in] obj 事件来源对象
    // @param[in] event 事件类型（Enter/Leave）
    void handleButtonEnterLeaveEvent(QObject* obj, QEvent* event);
};