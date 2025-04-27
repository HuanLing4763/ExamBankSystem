#pragma once

#include <QDialog>
#include "env/ExamEnvManager.h"
#include "ui_EnvSelectDialog.h"

class EnvSelectDialog : public QDialog
{
	Q_OBJECT

public:
	EnvSelectDialog(QWidget *parent = nullptr);
	~EnvSelectDialog();

	ExamEnvManager::EnvInfo selectedEnv() const { return m_selectedEnv; }

private:
	Ui::EnvSelectDialog ui;
	ExamEnvManager::EnvInfo m_selectedEnv;
};
