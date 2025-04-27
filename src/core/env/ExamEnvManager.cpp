#include <QCoreApplication>
#include <QIcon>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "ExamEnvManager.h"

ExamEnvManager& ExamEnvManager::getInstance()
{
	static ExamEnvManager instance;
	return instance;
}

ExamEnvManager::ExamEnvManager(QObject* parent) : QObject(parent), m_settings("ExamBankSystem", "ExamBankSystem")
{
	m_currentEnv.name = m_settings.value("ExamEnvName", "Python").toString();
	m_currentEnv.id = m_settings.value("ExamEnvID", 2).toInt();
	m_currentEnv.icon = QIcon(":/ExamBankSystem/resources/" + m_currentEnv.name + ".png");
	
	m_sharedFolderPath = m_settings.value("SharedFolderPath", "share").toString();
}

QString ExamEnvManager::getSharedFolderPath() const
{
	return QDir(QCoreApplication::applicationDirPath())
		.filePath(m_settings.value("SharedFolder", "share").toString());
}

ExamEnvManager::EnvInfo ExamEnvManager::currentEnvInfo() const
{
	return m_currentEnv;
}

QString ExamEnvManager::currentEnv() const
{
	return m_currentEnv.name;
}

int ExamEnvManager::currentEnvID() const
{
	return m_currentEnv.id;
}

QList<ExamEnvManager::EnvInfo> ExamEnvManager::envList()
{
    if (m_environments.isEmpty()) {
        // 创建本地图标缓存目录
        QDir cacheDir("icon_cache");
        if (!cacheDir.exists()) {
            cacheDir.mkpath(".");
        }

        QSettings settings("config.ini", QSettings::IniFormat, this);
        QString host = settings.value("Server/Host", "localhost").toString();
        int port = settings.value("Server/Port", 5000).toInt();

        QUrl url = QUrl(QString("http://%1:%2/subjects").arg(host).arg(port));
        QNetworkRequest request(url);

        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.get(request);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray subjects = QJsonDocument::fromJson(reply->readAll()).array();
            m_environments.clear();
            for (const auto& obj : subjects) {
                if (obj.isObject()) {
                    QJsonObject subject = obj.toObject();
                    EnvInfo env;
                    env.name = subject["subject_name"].toString();
                    env.id = subject["subject_id"].toInt();
                    QString iconName = subject["icon_name"].toString();

                    // 从图标名称中提取虚拟机名称
                    int dotIndex = iconName.lastIndexOf('.');
                    if (dotIndex != -1) {
                        env.vmName = iconName.left(dotIndex);
                    }
                    else {
                        env.vmName = iconName;
                    }

                    // 检查本地是否存在图标
                    QString iconPath = cacheDir.absoluteFilePath(iconName);
                    QFile iconFile(iconPath);
                    if (iconFile.exists()) {
                        // 使用本地图标
                        env.icon = QIcon(iconPath);
                    }
                    else {
                        // 从服务器下载图标
                        QUrl iconUrl(QString("http://%1:%2/subjects/get_icon/%3").arg(host).arg(port).arg(iconName));
                        QNetworkRequest iconRequest(iconUrl);
                        QNetworkReply* iconReply = manager.get(iconRequest);

                        QEventLoop iconLoop;
                        QObject::connect(iconReply, &QNetworkReply::finished, &iconLoop, &QEventLoop::quit);
                        iconLoop.exec();

                        if (iconReply->error() == QNetworkReply::NoError) {
                            // 保存图标到本地
                            if (iconFile.open(QIODevice::WriteOnly)) {
                                iconFile.write(iconReply->readAll());
                                iconFile.close();
                                env.icon = QIcon(iconPath);
                            }
                        }
                        iconReply->deleteLater();
                    }
                    m_environments.append(env);
                }
            }
        }
        reply->deleteLater();
    }

    return m_environments;
}

void ExamEnvManager::setCurrentEnv(const EnvInfo& env)
{
	if (m_currentEnv.id != env.id) {
		m_currentEnv = env;
		m_settings.setValue("ExamEnvName", m_currentEnv.name);
		m_settings.setValue("ExamEnvID", m_currentEnv.id);
		m_settings.sync();
		qDebug() << m_settings.value("ExamEnvName").toString();
		qDebug() << m_settings.value("ExamEnvID").toInt();
	}
}

QList<QPair<QString, QIcon>> ExamEnvManager::getQuestionTypes()
{
    if (questionTypes.isEmpty()) {
        // 创建本地图标缓存目录
        QDir cacheDir("icon_cache");
        if (!cacheDir.exists()) {
            cacheDir.mkpath(".");
        }

        // 向服务器请求题型数据
        QSettings settings("config.ini", QSettings::IniFormat, this);
        QString host = settings.value("Server/Host").toString();
        int port = settings.value("Server/Port").toInt();
        QString encodedExamType = QUrl::toPercentEncoding(m_currentEnv.name);
        QUrl url(QString("http://%1:%2/questions/types?exam_type=%3").arg(host).arg(port).arg(encodedExamType));
        QNetworkRequest request(url);

        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.get(request);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 200) {
                QByteArray body = reply->readAll();
                QList<QPair<QString, QIcon>> questionTypes;
                QJsonDocument doc = QJsonDocument::fromJson(body);
                QJsonArray array = doc.array();

                for (const auto& obj : array) {
                    QJsonObject obj_ = obj.toObject();
                    int subject_id = obj_.value("subject_id").toInt();
                    QString type = obj_.value("question_type").toString();
                    if (subject_id == 1) continue;

                    // 检查本地是否存在图标
                    QString iconPath = cacheDir.absoluteFilePath(type + ".png");
                    QFile iconFile(iconPath);
                    if (iconFile.exists()) {
                        // 使用本地图标
                        QIcon icon(iconPath);
                        questionTypes.append(QPair<QString, QIcon>(type, icon));
                    }
                    else {
                        // 从服务器下载图标
                        QUrl iconUrl(QString("http://%1:%2/questions/get_icon/%3.png").arg(host).arg(port).arg(type));
                        QNetworkRequest iconRequest(iconUrl);
                        QNetworkReply* iconReply = manager.get(iconRequest);

                        QEventLoop iconLoop;
                        QObject::connect(iconReply, &QNetworkReply::finished, &iconLoop, &QEventLoop::quit);
                        iconLoop.exec();

                        if (iconReply->error() == QNetworkReply::NoError) {
                            // 保存图标到本地
                            if (iconFile.open(QIODevice::WriteOnly)) {
                                iconFile.write(iconReply->readAll());
                                iconFile.close();
                                QIcon icon(iconPath);
                                questionTypes.append(QPair<QString, QIcon>(type, icon));
                            }
                        }
                        iconReply->deleteLater();
                    }
                }
                reply->deleteLater();
                return questionTypes;
            }
        }
        reply->deleteLater();
    }
    return questionTypes;
}

void ExamEnvManager::clearSettings() {
	m_settings.clear(); // 清除所有配置项
}