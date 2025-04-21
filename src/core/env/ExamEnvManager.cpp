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
					env.name = subject["name"].toString();
					env.id = subject["subject_id"].toInt();
					env.icon = QIcon(":/ExamBankSystem/resources/" + env.name + ".png");
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
		// 暂时用本地图标
		QList<QIcon> icons;
		icons.append(QIcon(":/ExamBankSystem/resources/单项选择"));
		icons.append(QIcon(":/ExamBankSystem/resources/程序填空"));
		icons.append(QIcon(":/ExamBankSystem/resources/程序修改"));
		icons.append(QIcon(":/ExamBankSystem/resources/程序设计"));

		// 向服务器请求题型数据
		QSettings settings("config.ini", QSettings::IniFormat, this);
		QString host = settings.value("Server/Host").toString();
		int port = settings.value("Server/Port").toInt();

		QUrl url(QString("http://%1:%2/questions/types?exam_type=%3").arg(host).arg(port).arg(m_currentEnv.name));
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
				int i = 0;   // 题型索引
				for (const auto& obj : array) {
					QJsonObject obj_ = obj.toObject();
					int subject_id = obj_.value("subject_id").toInt();
					QString type = obj_.value("question_type").toString();
					if (subject_id == 1) continue;
					// TODO: 动态获取题型图标（等待后端接口）
					questionTypes.append(QPair<QString, QIcon>(type, icons[i++]));
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