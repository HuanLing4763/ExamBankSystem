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

private slots:
	// @brief 处理题型选择按钮点击事件
	void onButtonClicked();
};