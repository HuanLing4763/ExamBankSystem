#include "VirtualBoxController.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <atlsafe.h>


VirtualBoxController* VirtualBoxController::instance = nullptr;
std::mutex VirtualBoxController::mtx;

VirtualBoxController* VirtualBoxController::getInstance() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = new VirtualBoxController();
    }
    return instance;
}

VirtualBoxController::VirtualBoxController(QObject* parent) : QObject(parent), m_comInitialized(false)
{
}

VirtualBoxController::~VirtualBoxController()
{
    uninitializeCOM();
}

bool VirtualBoxController::initializeCOM(QString& error)
{
    if (m_comInitialized)
        return true;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == S_FALSE)
    {
        // 当前线程已经初始化过COM库，直接使用
        qDebug() << "当前线程已初始化过COM库，直接使用。";
        m_comInitialized = true;
    }
    else if (hr != S_OK)
    {
        error = QString("COM初始化失败: 0x%1").arg(hr, 8, 16, QLatin1Char('0'));
        qDebug() << error;
        return false;
    }
    else
    {
        m_comInitialized = true;
    }

    hr = m_pClient.CoCreateInstance(__uuidof(VirtualBoxClient));
    if (hr != S_OK)
    {
        error = getCOMError(hr);
        qDebug() << error;
        if (hr != S_FALSE)
        {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    hr = m_pClient->get_VirtualBox(&m_pVirtualBox);
    if (hr != S_OK)
    {
        error = getCOMError(hr);
        qDebug() << error;
        if (hr != S_FALSE)
        {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    qDebug() << "COM初始化成功。";
    return true;
}

void VirtualBoxController::uninitializeCOM()
{
    if (m_comInitialized)
    {
        CoUninitialize();
        m_comInitialized = false;
        qDebug() << "COM库已释放。";
    }
}

bool VirtualBoxController::startVM(const QString& vmName, const QString& launchType) {
    QString error;
    HRESULT hr;
    CComPtr<IMachine> pMachine;

    // 转换虚拟机名称并输出调试信息
    std::wstring wstrVmName = vmName.toStdWString();
    CComBSTR bstrVmName(wstrVmName.c_str());
    qDebug() << "正在查找虚拟机: " << vmName << " (BSTR: " << QString::fromWCharArray(bstrVmName) << ")";

    hr = m_pVirtualBox->FindMachine(bstrVmName, &pMachine);
    if (FAILED(hr) || !pMachine) {
        QString error = "查找虚拟机失败，错误代码: 0x" + QString::number(hr, 16) + " - " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    // 创建新会话
    CComPtr<ISession> pSession;
    hr = m_pClient->get_Session(&pSession);
    if (FAILED(hr)) {
        QString error = "创建会话失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComSafeArray<BSTR> environment; // 创建空环境变量数组

    // 启动虚拟机进程
    CComPtr<IProgress> pProgress;
    hr = pMachine->LaunchVMProcess(
        pSession,
        CComBSTR(launchType.toStdWString().c_str()),
        environment,
        &pProgress
    );

    if (FAILED(hr)) {
        QString error = "启动虚拟机失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    // 等待启动完成
    hr = pProgress->WaitForCompletion(-1);
    if (FAILED(hr)) {
        QString error = "启动过程失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    // 获取控制台对象
    CComPtr<IConsole> pConsole;
    hr = pSession->get_Console(&pConsole);
    if (FAILED(hr)) {
        QString error = "获取控制台失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    // 转移引用到成员变量
    m_pConsole = pConsole;
    m_pSession = pSession;

    // 设置分辨率为1920x1080
    if (!setResolution(0, 1920, 1080)) {
        qDebug() << "设置分辨率失败，继续启动流程";
    }

    emit vmStarted();
    return true;
}

void VirtualBoxController::startVMAsync(const QString& vmName, const QString& launchType) {
    QtConcurrent::run([this, vmName, launchType]() {
        QString error;

        // 子线程初始化 COM 库
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            emit vmStartFailed("子线程 COM 初始化失败");
            return;
        }

        // 初始化当前实例的 COM 对象
        if (!initializeCOM(error)) {
            emit vmStartFailed("COM 对象初始化失败: " + error);
            CoUninitialize();
            return;
        }

        // 执行同步启动
        startVM(vmName, launchType);

        // 释放资源
        CoUninitialize();
        });
}

bool VirtualBoxController::setResolution(quint32 screenId, quint32 width, quint32 height)
{
    if (!m_pConsole) {
        emit vmStartFailed("控制台对象未初始化");
        return false;
    }

    CComPtr<IDisplay> pDisplay;
    HRESULT hr = m_pConsole->get_Display(&pDisplay);
    if (hr != S_OK) {
        emit vmStartFailed("获取显示接口失败: " + getCOMError(hr));
        return false;
    }

    // 设置分辨率
    hr = pDisplay->SetVideoModeHint(
        screenId,          // 屏幕ID
        VARIANT_TRUE,      // 启用屏幕
        VARIANT_FALSE,     // 不改变原点
        0, 0,              // 原点坐标
        width, height,     // 目标分辨率
        32,                // 位深
        VARIANT_TRUE       // 通知Guest
    );

    if (hr != S_OK) {
        emit vmStartFailed("设置分辨率失败: " + getCOMError(hr));
        return false;
    }

    return true;
}

bool VirtualBoxController::installGuestAdditions()
{
    if (!m_pConsole) {
        QString error = "控制台对象未初始化，无法安装Guest Additions";
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComPtr<IGuest> pGuest;
    HRESULT hr = m_pConsole->get_Guest(&pGuest);
    if (hr != S_OK)
    {
        QString error = "获取Guest接口失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComBSTR source(L"VBoxGuestAdditions.iso");
    CComPtr<IProgress> pProgress;
    hr = pGuest->UpdateGuestAdditions(source, nullptr, nullptr, &pProgress);
    if (hr != S_OK)
    {
        QString error = "安装Guest Additions失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    hr = pProgress->WaitForCompletion(-1);
    if (hr != S_OK)
    {
        QString error = "安装过程失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    return true;
}

QString VirtualBoxController::getCOMError(HRESULT hr) const
{
    _com_error err(hr);
    return QString::fromWCharArray(err.ErrorMessage());
}