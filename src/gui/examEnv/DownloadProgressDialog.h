#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

class DownloadProgressDialog : public QDialog {
    Q_OBJECT
public:
    enum State {
        StateDownloading,
        StateImporting
    };

    explicit DownloadProgressDialog(QWidget* parent = nullptr);
    void updateDownloadProgress(qint64 bytesRead, qint64 totalBytes);
    void startImport(const QString& vmName);
    void updateImportProgress(int percent);
    void finishDownload(bool success);
    void finishImport(bool success);

private:
    void handleFailure(const QString& errorMessage);

    QProgressBar* progressBar;
    QLabel* statusLabel;
    State currentState;
    QStyle* originalStyle;
};
