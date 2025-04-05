#include <QCoreApplication>
#include <QIcon>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <httplib.h>
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
    m_currentEnvName = m_settings.value("ExamEnvName", "Python").toString();
    m_currentEnvID = m_settings.value("ExamEnvID", "Python").toInt();
    m_sharedFolderPath = m_settings.value("SharedFolderPath", "share").toString();

    // 环境有效性验证
    auto exists = std::any_of(m_environments.begin(), m_environments.end(),
        [this](const QPair<QString, QIcon>& pair) { return pair.first == m_currentEnvName; });
    if (!exists && !m_environments.isEmpty()) {
        m_currentEnvName = m_environments.first().first;
    }
}

void ExamEnvManager::loadEnvironments()
{
    QSettings settings("config.ini", QSettings::IniFormat, this);
    QString host = settings.value("Server/Host").toString();
    int port = settings.value("Server/Port").toInt();

    httplib::Client cli(host.toStdString(), port);
    auto res = cli.Get("/subjects");
    if (res && res->status == 200) {
        auto subjects = res->body;
        QJsonDocument doc = QJsonDocument::fromJson(subjects.c_str());
        QJsonArray array = doc.array();
        for (auto obj : array) {
            m_currentEnvID = obj.toObject().value("subject_id").toInt();
            m_currentEnvName = obj.toObject().value("name").toString();

            m_environments.append(QPair<QString, QIcon>(m_currentEnvName, QIcon(":/ExamBankSystem/resources/" + m_currentEnvName + ".png")));
        }
    }
}

QPair<QString, QIcon> ExamEnvManager::currentEnvPair() const
{
    // 在环境列表中查找当前配置
    auto it = std::find_if(m_environments.begin(), m_environments.end(),
        [this](const auto& p) { return p.first == m_currentEnvName; });
    return (it != m_environments.end()) ? *it : qMakePair(QString(), QIcon());
}

QString ExamEnvManager::getSharedFolderPath() const {
    QString folder = m_settings.value("SharedFolder", "share").toString();
    return QDir(QCoreApplication::applicationDirPath()).filePath(folder);
}

QString ExamEnvManager::currentEnv() const
{
    return m_currentEnvName;
}

int ExamEnvManager::currentEnvID() const
{
    return m_currentEnvID;
}

QList<QPair<QString, QIcon>> ExamEnvManager::availableEnvs() const
{
    return m_environments;
}

void ExamEnvManager::setCurrentEnv(const QString& env)
{
    if (m_currentEnvName != env) {
        m_currentEnvName = env;
        m_settings.setValue("ExamEnvName", m_currentEnvName);
        m_settings.setValue("ExamEnvID", m_currentEnvID);

        // 查找并通知环境变更
        auto it = std::find_if(m_environments.begin(), m_environments.end(),
            [&](const QPair<QString, QIcon>& pair) { return pair.first == env; });
        if (it != m_environments.end()) {
            emit envDetailChanged(*it);
        }
    }
}