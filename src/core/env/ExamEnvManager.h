#pragma once

#include <QObject>
#include <QSettings>
#include <QIcon>
#include <QPair>
#include <QList>

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

    struct EnvInfo {
        QString name;
        int id;
        QIcon icon;
    };

    QString currentEnv() const;  // 获取当前环境名称
    int currentEnvID() const;  // 获取当前环境ID
    QString getSharedFolderPath() const;  // 获取共享文件夹绝对路径
    QList<EnvInfo> envList();  // 获取所有可用环境列表
    EnvInfo currentEnvInfo() const;  // 获取当前环境的完整信息
    QList<QPair<QString, QIcon>> getQuestionTypes();  // 获取题型数据集合
    void clearSettings();  // 清除配置

public slots:
    // @brief 切换考试环境
    void setCurrentEnv(const EnvInfo& env);

private:
    explicit ExamEnvManager(QObject* parent = nullptr);

    QSettings m_settings;  // 配置存储对象
    EnvInfo m_currentEnv;  // 当前环境
    QString m_sharedFolderPath;  // 共享文件夹绝对路径
    QList<EnvInfo> m_environments;  // 可用环境列表
    QList<QPair<QString, QIcon>> questionTypes;  // 题型数据集合
};