#pragma one

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>

class DownloadProgressDialog : public QDialog
{
    Q_OBJECT
public:
    enum State {
        StateDownloading,
        StateImporting
    };

    DownloadProgressDialog(QWidget* parent = nullptr);
    ~DownloadProgressDialog();

    void updateDownloadProgress(qint64 bytesRead, qint64 totalBytes);
    void startImport(const QString& vmName);
    void updateImportProgress(int percent);
    void finishDownload(bool success);
    void finishImport(bool success);

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
    State currentState;
};