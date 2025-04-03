#pragma once

#include <QObject>
#include <QSettings>
#include <QIcon>
#include <QPair>
#include <QList>
#include <algorithm>

/**
 * @brief 考试环境管理器（单例模式）
 * @details 负责管理虚拟考试环境配置，包括：
 * - 可用环境列表维护
 * - 当前环境状态管理
 * - 配置持久化存储
 */
class ExamEnvManager : public QObject
{
    Q_OBJECT
public:
    static ExamEnvManager& getInstance();

    QString currentEnv() const;  // 获取当前环境标识
    QString getSharedFolderPath() const;  // 获取共享文件夹绝对路径
    QList<QPair<QString, QIcon>> availableEnvs() const;  // 获取所有可用环境列表
    QPair<QString, QIcon> currentEnvPair() const;  // 获取当前环境的完整信息

public slots:
    // @brief 切换考试环境（自动持久化存储）
    void setCurrentEnv(const QString& env);

signals:
    // @brief 环境信息发生变化
    void envDetailChanged(const QPair<QString, QIcon>& env);

private:
    explicit ExamEnvManager(QObject* parent = nullptr);
    void loadEnvironments();  // 加载所有可用环境信息

    QSettings m_settings;  // 配置存储对象
    QString m_currentEnv;  // 当前环境标识
    QString m_sharedFolderPath;  // 共享文件夹绝对路径
    QList<QPair<QString, QIcon>> m_environments;  // 可用环境列表
};