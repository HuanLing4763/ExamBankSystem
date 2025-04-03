#include <QCoreApplication>
#include <QIcon>
#include <QDir>
#include "ExamEnvManager.h"

ExamEnvManager& ExamEnvManager::getInstance()
{
    static ExamEnvManager instance;
    return instance;
}

ExamEnvManager::ExamEnvManager(QObject* parent)
    : QObject(parent)
{
    // 加载配置
    loadEnvironments();
    m_currentEnv = m_settings.value("ExamEnv", "Python").toString();
    m_sharedFolderPath = m_settings.value("SharedFolderPath", "share").toString();

    // 环境有效性验证
    auto exists = std::any_of(m_environments.begin(), m_environments.end(),
        [this](const QPair<QString, QIcon>& pair) { return pair.first == m_currentEnv; });
    if (!exists && !m_environments.isEmpty()) {
        m_currentEnv = m_environments.first().first;
    }
}

void ExamEnvManager::loadEnvironments()
{
    // TODO: 需替换硬编码测试数据为动态加载
    m_environments.append(QPair<QString, QIcon>("C", QIcon(":/ExamBankSystem/resources/c++语言")));
    m_environments.append(QPair<QString, QIcon>("C++", QIcon(":/ExamBankSystem/resources/c++语言")));
    m_environments.append(QPair<QString, QIcon>("Python", QIcon(":/ExamBankSystem/resources/python")));
}

QPair<QString, QIcon> ExamEnvManager::currentEnvPair() const
{
    // 在环境列表中查找当前配置
    auto it = std::find_if(m_environments.begin(), m_environments.end(),
        [this](const auto& p) { return p.first == m_currentEnv; });
    return (it != m_environments.end()) ? *it : qMakePair(QString(), QIcon());
}

QString ExamEnvManager::getSharedFolderPath() const {
    QString folder = m_settings.value("SharedFolder", "share").toString();
    return QDir(QCoreApplication::applicationDirPath()).filePath(folder);
}

QString ExamEnvManager::currentEnv() const
{
    return m_currentEnv;
}

QList<QPair<QString, QIcon>> ExamEnvManager::availableEnvs() const
{
    return m_environments;
}

void ExamEnvManager::setCurrentEnv(const QString& env)
{
    if (m_currentEnv != env) {
        m_currentEnv = env;
        m_settings.setValue("ExamEnv", m_currentEnv);  // 保存配置

        // 查找并通知环境变更
        auto it = std::find_if(m_environments.begin(), m_environments.end(),
            [&](const QPair<QString, QIcon>& pair) { return pair.first == env; });
        if (it != m_environments.end()) {
            emit envDetailChanged(*it);
        }
    }
}