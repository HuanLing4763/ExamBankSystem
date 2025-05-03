#include "DownloadProgressDialog.h"
#include <QPainter>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <QStylePainter>
#include <QTimer>
#include <QVBoxLayout>


DownloadProgressDialog::DownloadProgressDialog(QWidget* parent)
    : QDialog(parent),
    currentState(StateDownloading),
    statusLabel(new QLabel("下载中...")),
    progressBar(new QProgressBar)
{
    setWindowTitle("下载并导入虚拟机");
    setFixedSize(300, 120);

    progressBar->setRange(0, 100);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);

    setWindowModality(Qt::WindowModal);
    originalStyle = progressBar->style();
}

void DownloadProgressDialog::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes) {
    currentState = StateDownloading;
    if (totalBytes <= 0) return;
    int progress = static_cast<int>((bytesRead * 100.0) / totalBytes);
    progressBar->setRange(0, 100);
    progressBar->setValue(progress);
    statusLabel->setText(QString("下载进度: %1% (%2/%3 MB)")
        .arg(progress)
        .arg(bytesRead / 1024 / 1024)
        .arg(totalBytes / 1024 / 1024));
}

void DownloadProgressDialog::startImport(const QString& vmName) {
    currentState = StateImporting;
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    statusLabel->setText(QString("开始导入虚拟机 %1...").arg(vmName));
}

void DownloadProgressDialog::updateImportProgress(int percent) {
    if (currentState != StateImporting) return;
    progressBar->setValue(percent);
    statusLabel->setText(QString("导入进度: %1%").arg(percent));
}

void DownloadProgressDialog::finishDownload(bool success) {
    if (success) {
        statusLabel->setText("下载完成！开始导入...");
    }
    else {
        handleFailure("下载失败，请重试！");
    }
}

void DownloadProgressDialog::finishImport(bool success) {
    if (success) {
        statusLabel->setText("导入完成！");
    }
    else {
        handleFailure("导入失败，请检查日志。");
    }
}

void DownloadProgressDialog::handleFailure(const QString& errorMessage) {
    statusLabel->setText(errorMessage);
    progressBar->setEnabled(false);

    class ProgressBarColorProxy : public QProxyStyle {
        QColor m_chunkColor;
        QStyle* m_baseStyle;

    public:
        ProgressBarColorProxy(QStyle* baseStyle, const QColor& color)
            : m_baseStyle(baseStyle), m_chunkColor(color) {}

        void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override {
            if (element == QStyle::CE_ProgressBarContents && m_chunkColor.isValid()) {
                const QStyleOptionProgressBar* progressOpt = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
                if (!progressOpt) {
                    m_baseStyle->drawControl(element, option, painter, widget);
                    return;
                }

                qreal fraction = 0.0;
                if (progressOpt->maximum != progressOpt->minimum) {
                    fraction = qreal(progressOpt->progress - progressOpt->minimum) / (progressOpt->maximum - progressOpt->minimum);
                }

                QRect chunkRect = progressOpt->rect.adjusted(2, 2, -2, -2);
                chunkRect.setWidth(qRound(chunkRect.width() * fraction));

                painter->fillRect(chunkRect, m_chunkColor);
            }
            else {
                m_baseStyle->drawControl(element, option, painter, widget);
            }
        }
    };

    ProgressBarColorProxy* errorProxy = new ProgressBarColorProxy(originalStyle, QColor("#ff0000"));
    errorProxy->setParent(progressBar);
    progressBar->setStyle(errorProxy);
    progressBar->update(); 
}
