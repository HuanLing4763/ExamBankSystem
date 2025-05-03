#include "network/NetworkManager.h"
#include "VirtualBoxController.h"
#include <atlsafe.h>
#include <QDebug>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QtConcurrent/QtConcurrent>

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

void VirtualBoxController::createVM(const QString& ovaFilePath) {
    QString error;
    if (!initializeCOM(error)) {
        emit importFinished(false);
        return;
    }

    CComPtr<IAppliance> appliance;
    HRESULT hr = m_pVirtualBox->CreateAppliance(&appliance);
    if (FAILED(hr)) {
        emit importFinished(false);
        return;
    }

    CComBSTR bstrFilePath(ovaFilePath.toStdWString().c_str());
    CComPtr<IProgress> readProgress;
    hr = appliance->Read(bstrFilePath, &readProgress);
    if (FAILED(hr)) {
        qDebug() << "读取 OVA 文件失败: " << getCOMError(hr);
        emit importFinished(false);
        return;
    }

    // 实时获取读取进度（0%~30%）
    BOOL fReadCompleted = FALSE;
    while (!fReadCompleted) {
        hr = readProgress->get_Completed(&fReadCompleted);
        if (FAILED(hr)) {
            emit importFinished(false);
            return;
        }
        // 获取当前阶段进度百分比（假设总进度30%）
        ULONG percent;
        readProgress->get_Percent(&percent);
        emit vmImportProgress(percent * 30 / 100); // 映射到0%~30%
        Sleep(100);
    }
    emit vmImportProgress(30); // 阶段1完成

    // 解析配置（30%~50%）
    hr = appliance->Interpret();
    if (FAILED(hr)) {
        qDebug() << "解析 OVA 文件失败: " << getCOMError(hr);
        emit importFinished(false);
        return;
    }
    emit vmImportProgress(50); // 阶段2完成

    // 导入虚拟机（50%~100%）
    SAFEARRAY* aOptions = SafeArrayCreateVector(VT_VARIANT, 0, 0);
    if (!aOptions) {
        emit importFinished(false);
        return;
    }

    CComPtr<IProgress> importProgress;
    hr = appliance->ImportMachines(aOptions, &importProgress);
    SafeArrayDestroy(aOptions); // 确保无论成功与否都释放

    if (FAILED(hr)) {
        emit importFinished(false);
        return;
    }

    BOOL fImportCompleted = FALSE;
    while (!fImportCompleted) {
        hr = importProgress->get_Completed(&fImportCompleted);
        if (FAILED(hr)) {
            emit importFinished(false);
            return;
        }
        // 获取当前阶段进度百分比（映射到50%~100%）
        ULONG percent;
        importProgress->get_Percent(&percent);
        emit vmImportProgress(50 + percent * 50 / 100);
        Sleep(100);
    }

    emit vmImportProgress(100);
    emit importFinished(true);

    uninitializeCOM();
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
        if (!initializeCOM(error)) {
            emit vmStartFailed("COM 对象初始化失败: " + error);
            uninitializeCOM();
            return;
        }

        startVM(vmName, launchType);

        uninitializeCOM();
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

void VirtualBoxController::downloadVMImage(const QString& imageName)
{
    QSettings settings("config.ini", QSettings::IniFormat, this);
    QString host = settings.value("Server/Host").toString();
    int port = settings.value("Server/Port").toInt();

    QUrl url = QUrl(QString("http://%1:%2/vm-images/download/%3").arg(host).arg(port).arg(imageName));
    QNetworkRequest request(url);

    QNetworkAccessManager& manager = NetworkManager::instance();
    QNetworkReply* reply = manager.get(request);
    reply->setParent(this);

    // 在reply对象上存储状态
    QSharedPointer<QFile> file(new QFile);
    reply->setProperty("downloadFile", QVariant::fromValue(file));
    reply->setProperty("imageName", imageName);

    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        auto file = reply->property("downloadFile").value<QSharedPointer<QFile>>();
        QString imageName = reply->property("imageName").toString();

        if (!file->isOpen()) {  // 首次触发时初始化
            // 从响应头提取文件名
            QVariant headerValue = reply->header(QNetworkRequest::ContentDispositionHeader);
            QString downloadedFileName;
            if (!headerValue.isNull()) {
                QString contentDisposition = headerValue.toString();
                int filenameStart = contentDisposition.indexOf("filename=");
                if (filenameStart != -1) {
                    QString filenamePart = contentDisposition.mid(filenameStart + 9).trimmed();
                    downloadedFileName = filenamePart.startsWith('"')
                        ? filenamePart.mid(1, filenamePart.size() - 2)
                        : filenamePart;
                }
            }
            // 未找到文件名则使用默认名称
            if (downloadedFileName.isEmpty()) {
                downloadedFileName = imageName + ".ova";
            }
            QString filePath = VIRTUALBOX_IMAGES_DIR + downloadedFileName;
            file->setFileName(filePath);

            // 打开文件失败处理
            if (!file->open(QIODevice::WriteOnly)) {
                qDebug() << "文件打开失败，路径：" << filePath << "错误：" << file->errorString();
                reply->abort();
                return;
            }
            reply->setProperty("downloadedFileName", downloadedFileName);  // 保存文件名到reply属性
        }

        // 写入数据
        if (file->isOpen()) {
            QByteArray data = reply->readAll();
            if (!data.isEmpty() && file->write(data) == -1) {
                qDebug() << "文件写入失败，错误：" << file->errorString();
                reply->abort();
            }
        }
        });

    // 下载完成后清理资源
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto file = reply->property("downloadFile").value<QSharedPointer<QFile>>();
        QString downloadedFileName = reply->property("downloadedFileName").toString();
        bool success = false;

        if (reply->error() == QNetworkReply::NoError) {
            if (file->isOpen()) {
                // 写入剩余数据
                QByteArray remainingData = reply->readAll();
                if (!remainingData.isEmpty()) {
                    file->write(remainingData);
                }
                file->close();
                success = true;
            }
        }
        else {
            qDebug() << "下载失败: " << reply->errorString();
            if (file->isOpen()) {
                file->close();
                QFile::remove(file->fileName());  // 删除不完整文件
            }
        }

        emit downloadFinished(success, downloadedFileName);
        reply->deleteLater();
        });

    // 连接下载进度信号
    connect(reply, &QNetworkReply::downloadProgress, this, &VirtualBoxController::downloadProgress);
}