#include "VirtualBoxController.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <atlsafe.h>
#include <QNetworkReply>
#include <QNetworkDiskCache>

const QString VIRTUALBOX_IMAGES_DIR = "E:/Project/CPP/ExamBankSystem/images/";

VirtualBoxController* VirtualBoxController::instance = nullptr;
std::mutex VirtualBoxController::mtx;

VirtualBoxController* VirtualBoxController::getInstance()
{
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
        qDebug() << "当前线程已初始化过 COM 库，直接使用。";
        m_comInitialized = true;
    }
    else if (hr != S_OK)
    {
        error = QString("COM 初始化失败: 0x%1").arg(hr, 8, 16, QLatin1Char('0'));
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

    qDebug() << "COM 初始化成功。";
    return true;
}

void VirtualBoxController::uninitializeCOM()
{
    if (m_comInitialized)
    {
        CoUninitialize();
        m_comInitialized = false;
        qDebug() << "COM 库已释放。";
    }
}

HRESULT VirtualBoxController::createVM(const QString& ovaFilePath)
{
    QString error;
    if (!initializeCOM(error)) {
        emit importFinished(false);
        return E_FAIL;
    }

    CComPtr<IAppliance> appliance;
    HRESULT hr = m_pVirtualBox->CreateAppliance(&appliance);
    if (FAILED(hr)) {
        emit importFinished(false);
        return hr;
    }

    CComBSTR bstrFilePath(ovaFilePath.toStdWString().c_str());
    CComPtr<IProgress> readProgress;
    hr = appliance->Read(bstrFilePath, &readProgress);
    if (FAILED(hr)) {
        emit importFinished(false);
        return hr;
    }

    // 等待读取完成
    BOOL fReadCompleted = FALSE;
    while (!fReadCompleted) {
        hr = readProgress->get_Completed(&fReadCompleted);
        if (FAILED(hr)) {
            emit importFinished(false);
            return hr;
        }
        Sleep(100);
    }

    hr = appliance->Interpret();
    if (FAILED(hr)) {
        emit importFinished(false);
        return hr;
    }

    if (hr == VBOX_E_NOT_SUPPORTED) {
        qDebug() << "当前版本暂不支持 OVA 格式（TAR 打包），请使用解压后的 OVF 文件";
        emit importFinished(false);
        return hr;
    }

    // 创建空的 SAFEARRAY 作为 ImportMachines 的 aOptions 参数
    SAFEARRAY* aOptions = SafeArrayCreateVector(VT_VARIANT, 0, 0);
    if (!aOptions) {
        qDebug() << "创建 SAFEARRAY 失败";
        emit importFinished(false);
        return E_OUTOFMEMORY;
    }

    CComPtr<IProgress> importProgress;
    hr = appliance->ImportMachines(aOptions, &importProgress);
    // 释放 SAFEARRAY
    SafeArrayDestroy(aOptions);

    if (FAILED(hr)) {
        emit importFinished(false);
        return hr;
    }

    BOOL fImportCompleted = FALSE;
    while (!fImportCompleted) {
        hr = importProgress->get_Completed(&fImportCompleted);
        if (FAILED(hr)) {
            emit importFinished(false);
            return hr;
        }
        if (fImportCompleted) {
            emit vmImportProgress(100);
            emit importFinished(true);
        }
        Sleep(100);
    }

    return hr;
}

bool VirtualBoxController::startVM(const QString& vmName, const QString& launchType)
{
    QString error;
    HRESULT hr;
    CComPtr<IMachine> pMachine;

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

    CComPtr<ISession> pSession;
    hr = m_pClient->get_Session(&pSession);
    if (FAILED(hr)) {
        QString error = "创建会话失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComSafeArray<BSTR> environment;

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

    hr = pProgress->WaitForCompletion(-1);
    if (FAILED(hr)) {
        QString error = "启动过程失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComPtr<IConsole> pConsole;
    hr = pSession->get_Console(&pConsole);
    if (FAILED(hr)) {
        QString error = "获取控制台失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    m_pConsole = pConsole;
    m_pSession = pSession;

    if (!setResolution(0, 1920, 1080)) {
        qDebug() << "设置分辨率失败，继续启动流程";
    }

    emit vmStarted();
    return true;
}

void VirtualBoxController::startVMAsync(const QString& vmName, const QString& launchType)
{
    QtConcurrent::run([this, vmName, launchType]() {
        QString error;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            emit vmStartFailed("子线程 COM 初始化失败");
            return;
        }

        if (!initializeCOM(error)) {
            emit vmStartFailed("COM 对象初始化失败: " + error);
            CoUninitialize();
            return;
        }

        startVM(vmName, launchType);

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

    hr = pDisplay->SetVideoModeHint(
        screenId,
        VARIANT_TRUE,
        VARIANT_FALSE,
        0, 0,
        width, height,
        32,
        VARIANT_TRUE
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
        QString error = "控制台对象未初始化，无法安装 Guest Additions";
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComPtr<IGuest> pGuest;
    HRESULT hr = m_pConsole->get_Guest(&pGuest);
    if (hr != S_OK)
    {
        QString error = "获取 Guest 接口失败: " + getCOMError(hr);
        qDebug() << error;
        emit vmStartFailed(error);
        return false;
    }

    CComBSTR source(L"VBoxGuestAdditions.iso");
    CComPtr<IProgress> pProgress;
    hr = pGuest->UpdateGuestAdditions(source, nullptr, nullptr, &pProgress);
    if (hr != S_OK)
    {
        QString error = "安装 Guest Additions 失败: " + getCOMError(hr);
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

bool VirtualBoxController::isVMExists(const QString& vmName)
{
    QString error;
    if (!initializeCOM(error)) {
        qDebug() << "COM 初始化失败，无法检查虚拟机是否存在: " << error;
        return false;
    }

    std::wstring wstrVmName = vmName.toStdWString();
    CComBSTR bstrVmName(wstrVmName.c_str());
    CComPtr<IMachine> pMachine;
    HRESULT hr = m_pVirtualBox->FindMachine(bstrVmName, &pMachine);
    if (SUCCEEDED(hr)) {
        return true;
    }
    else if (hr == VBOX_E_OBJECT_NOT_FOUND) {
        return false;
    }
    else {
        qDebug() << "查找虚拟机时发生错误: " << getCOMError(hr);
        return false;
    }
}

bool VirtualBoxController::downloadVMImage(const QString& imageName)
{
//TODO;实现虚拟机镜像下载
}