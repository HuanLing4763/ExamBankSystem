#include <QVBoxLayout>
#include <QTimer>
#include "DownloadProgressDialog.h"

DownloadProgressDialog::DownloadProgressDialog(QWidget* parent)
    : QDialog(parent), currentState(StateDownloading)
{
    setWindowTitle("下载并导入虚拟机");
    setFixedSize(300, 120);

    progressBar = new QProgressBar(this);
    statusLabel = new QLabel("下载中...", this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
}

DownloadProgressDialog::~DownloadProgressDialog()
{}

void DownloadProgressDialog::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (currentState == StateDownloading) {
        progressBar->setMaximum(totalBytes);
        progressBar->setValue(bytesRead);
        statusLabel->setText(QString("下载进度: %1/%2 MB").arg(bytesRead / 1024 / 1024).arg(totalBytes / 1024 / 1024));
    }
}

void DownloadProgressDialog::startImport(const QString& vmName)
{
    currentState = StateImporting;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    statusLabel->setText(QString("开始导入虚拟机 %1...").arg(vmName));
}

void DownloadProgressDialog::updateImportProgress(int percent)
{
    if (currentState == StateImporting) {
        progressBar->setValue(percent);
        statusLabel->setText(QString("导入进度: %1%").arg(percent));
    }
}

void DownloadProgressDialog::finishDownload(bool success)
{
    if (success) {
        statusLabel->setText("下载完成！开始导入虚拟机...");
    }
    else {
        statusLabel->setText("下载失败，请重试！");
    }
    progressBar->setEnabled(false);
}

void DownloadProgressDialog::finishImport(bool success)
{
    if (success) {
        statusLabel->setText("导入完成！");
    }
    else {
        statusLabel->setText("导入失败，请检查日志。");
    }
    progressBar->setEnabled(false);
    QTimer::singleShot(2000, this, &QDialog::accept);
}
