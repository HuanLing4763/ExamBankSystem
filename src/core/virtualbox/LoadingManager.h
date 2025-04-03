#pragma once
#include <QObject>
#include <QDialog>
#include <QTimer>
#include <QQuickWidget>

class LoadingManager : public QObject {
    Q_OBJECT
private:
    explicit LoadingManager(QObject* parent = nullptr);
    ~LoadingManager();
    LoadingManager(const LoadingManager&) = delete;
    LoadingManager& operator=(const LoadingManager&) = delete;

    static LoadingManager* instance;

    QDialog* m_loadingDialog = nullptr;
    QQuickWidget* m_qmlWidget = nullptr;

signals:
    void cancelRequested(); // 取消信号
public slots:
    void handleQmlCancel() { emit cancelRequested(); } // QML取消信号转发

public:
    static LoadingManager* getInstance();
    void show(int timeoutMs = 10000);  // 显示加载界面，默认超时10秒
    void hide();                       // 手动关闭加载界面
};