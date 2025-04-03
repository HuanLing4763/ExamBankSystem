#pragma once

#include <QWidget>
#include "ui_PageSpecial.h"

/**
 * @brief 专项练习页面，展示题目类型选择入口
 * @details 动态生成题型选择按钮，支持点击进入具体题型练习
 */
class PageSpecial : public QWidget
{
	Q_OBJECT

public:
	PageSpecial(QWidget *parent = nullptr);
	~PageSpecial();

private:
	Ui::SpecialTrainingPage ui;
	QList<QPair<QString, QIcon>> questionTypes;  // 题型数据集合（类型名称+图标）

	// @brief 获取题型配置数据（当前为临时实现）
	QList<QPair<QString, QIcon>> getQuestionTypes();

private slots:
	// @brief 处理题型选择按钮点击事件
	void onButtonClicked();
};