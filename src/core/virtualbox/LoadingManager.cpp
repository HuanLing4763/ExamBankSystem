#include "LoadingManager.h"
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QQuickItem>

LoadingManager* LoadingManager::instance = nullptr;

LoadingManager::LoadingManager(QObject* parent) : QObject(parent) {
    m_loadingDialog = new QDialog(nullptr);
#ifdef QT_DEBUG
    m_loadingDialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
#else
    m_loadingDialog->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_loadingDialog->setGeometry(QGuiApplication::primaryScreen()->geometry());
#endif

    m_qmlWidget = new QQuickWidget(m_loadingDialog);
    m_qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qmlWidget->setClearColor(Qt::transparent);
    m_qmlWidget->setSource(QUrl("qrc:/ExamBankSystem/qml/Loading.qml"));

    QVBoxLayout* layout = new QVBoxLayout(m_loadingDialog);
    layout->addWidget(m_qmlWidget);
}

LoadingManager::~LoadingManager() {
    if (m_loadingDialog) {
        m_loadingDialog->close();
        delete m_loadingDialog;
    }
    if (m_qmlWidget) {
        delete m_qmlWidget;
    }
}

LoadingManager* LoadingManager::getInstance() {
    if (instance == nullptr) {
        instance = new LoadingManager();
    }
    return instance;
}

void LoadingManager::show(int timeoutMs) {
#ifdef QT_DEBUG
    m_loadingDialog->show();
#else
    m_loadingDialog->showFullScreen();
#endif
}

void LoadingManager::hide() {
    m_loadingDialog->close();
}