#ifndef PROTONDOWNLOADWIDGET_H
#define PROTONDOWNLOADWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;
class QProgressBar;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QFrame;

struct ProtonReleaseInfo {
    QString name;
    QString tagName;
    QString downloadUrl;
    qint64 size{0};
    bool isInstalled{false};
};

class ProtonRowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProtonRowWidget(const ProtonReleaseInfo &info, QWidget *parent = nullptr);

    const ProtonReleaseInfo &info() const { return m_info; }
    void setInstalled(bool installed);
    void updateProgress(qint64 received, qint64 total);
    void setExtracting();
    void setDownloading(bool downloading);
    void setError(const QString &msg);

signals:
    void downloadRequested(const ProtonReleaseInfo &info);
    void cancelRequested();

private:
    void updateIcon();

    ProtonReleaseInfo m_info;
    QLabel *m_iconLabel{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLabel *m_sizeLabel{nullptr};
    QProgressBar *m_progressBar{nullptr};
    QPushButton *m_actionButton{nullptr};
    bool m_isDownloading{false};
};

class ProtonDownloadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProtonDownloadWidget(QWidget *parent = nullptr);
    ~ProtonDownloadWidget() override;

    void refreshReleases();

private slots:
    void onReleasesReplyFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadReadyRead();
    void onDownloadFinished();
    void startDownload(const ProtonReleaseInfo &info);
    void cancelCurrentDownload();

private:
    void setupUi();
    void populateRows(const QList<ProtonReleaseInfo> &releases);
    QList<ProtonReleaseInfo> getFallbackReleases() const;
    bool checkIfInstalled(const QString &name, const QString &tagName) const;
    QString getSteamToolsDir() const;

    QNetworkAccessManager *m_netManager{nullptr};
    QNetworkReply *m_releasesReply{nullptr};
    QNetworkReply *m_downloadReply{nullptr};
    std::unique_ptr<QFile> m_downloadFile;
    QString m_downloadTempPath;
    ProtonReleaseInfo m_activeDownloadInfo;
    ProtonRowWidget *m_activeRow{nullptr};

    void updateUmuCard();

    QFrame *m_umuCard{nullptr};
    QLabel *m_umuStatusLabel{nullptr};
    QPushButton *m_umuButton{nullptr};

    QVBoxLayout *m_rowsLayout{nullptr};
    QLabel *m_statusLabel{nullptr};
    QPushButton *m_refreshButton{nullptr};
};

#endif // PROTONDOWNLOADWIDGET_H
