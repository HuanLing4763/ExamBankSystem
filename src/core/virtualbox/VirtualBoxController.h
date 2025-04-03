#pragma once
#include <atlbase.h>
#include <comdef.h>
#include <VirtualBox.h>
#include <QString>
#include <QObject>
#include <atomic>
#include <mutex>

class VirtualBoxController : public QObject {
    Q_OBJECT
private:
    VirtualBoxController(QObject* parent = nullptr);
    VirtualBoxController(const VirtualBoxController&) = delete;
    VirtualBoxController& operator=(const VirtualBoxController&) = delete;

public:
    ~VirtualBoxController();

    static VirtualBoxController* getInstance();

    // COM库管理
    bool initializeCOM(QString& error);
    void uninitializeCOM();

    // 虚拟机操作
    bool startVM(const QString& vmName, const QString& launchType = "gui");
    bool setResolution(quint32 screenId, quint32 width, quint32 height);
    bool installGuestAdditions();

public slots:
    void startVMAsync(const QString& vmName, const QString& launchType = "gui");

private:
    // COM对象成员
    CComPtr<IVirtualBoxClient> m_pClient;
    CComPtr<IVirtualBox> m_pVirtualBox;
    CComPtr<ISession> m_pSession;
    CComPtr<IConsole> m_pConsole;
    bool m_comInitialized = false;

    // 单例实例
    static VirtualBoxController* instance;
    static std::mutex mtx; // 确保线程安全

    // 错误处理
    QString getCOMError(HRESULT hr) const;
    std::atomic<bool> m_abortFlag{ false };

signals:
    void vmStarted();
    void vmStartFailed(const QString& error);
};