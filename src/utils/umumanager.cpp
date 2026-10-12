#include "umumanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProgressDialog>
#include <QWidget>

static QString formatBytes(qint64 bytes)
{
    if (bytes <= 0) return QStringLiteral("0 KB");
    double kb = static_cast<double>(bytes) / 1024.0;
    if (kb >= 1024.0) {
        return QStringLiteral("%1 MB").arg(kb / 1024.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 KB").arg(kb, 0, 'f', 0);
}

UmuManager::UmuManager(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

UmuManager::~UmuManager()
{
    if (m_dialog) {
        m_dialog->deleteLater();
    }
}

QString UmuManager::sparkUmuPath()
{
    return QDir::homePath() + QStringLiteral("/.local/share/spark/umu/umu-run");
}

QString UmuManager::findUmuBin()
{
    QString sysBin = QStandardPaths::findExecutable(QStringLiteral("umu-run"));
    if (!sysBin.isEmpty() && QFileInfo(sysBin).isExecutable()) {
        return sysBin;
    }

    QString localSpark = sparkUmuPath();
    if (QFile::exists(localSpark) && QFileInfo(localSpark).isExecutable()) {
        return localSpark;
    }

    return QString();
}

bool UmuManager::isUmuAvailable()
{
    return !findUmuBin().isEmpty();
}

void UmuManager::ensureUmuInstalled(QWidget *parent, std::function<void(bool success)> callback)
{
    if (isUmuAvailable()) {
        if (callback) {
            callback(true);
        }
        return;
    }

    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(tr("UMU Launcher Required"));
    msgBox.setText(tr("<b>UMU Launcher</b> is required to run Proton games stably on Linux (using Steam Linux Runtime containers)."));
    msgBox.setInformativeText(tr("UMU Launcher is not currently installed on your system.\nWould you like Spark to download and install it now? (~400 KB)"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setIcon(QMessageBox::Question);

    if (msgBox.exec() != QMessageBox::Yes) {
        if (callback) {
            callback(false);
        }
        return;
    }

    auto *manager = new UmuManager(parent);
    manager->startDownload(parent, callback);
}

void UmuManager::startDownload(QWidget *parent, std::function<void(bool success)> callback)
{
    m_callback = callback;

    if (parent) {
        m_dialog = new QProgressDialog(parent);
        m_dialog->setWindowTitle(tr("UMU Launcher"));
        m_dialog->setLabelText(tr("Checking latest release of UMU Launcher..."));
        m_dialog->setRange(0, 100);
        m_dialog->setValue(0);
        m_dialog->setWindowModality(Qt::WindowModal);
        m_dialog->setCancelButtonText(tr("Cancel"));
        m_dialog->setMinimumDuration(0);
        m_dialog->show();

        connect(m_dialog, &QProgressDialog::canceled, this, [this]() {
            if (m_callback) {
                m_callback(false);
            }
            emit finished(false, tr("Download cancelled by user."));
            deleteLater();
        });
    }

    fetchReleaseInfo();
}

void UmuManager::fetchReleaseInfo()
{
    QNetworkRequest req(QUrl(QStringLiteral("https://api.github.com/repos/Open-Wine-Components/umu-launcher/releases/latest")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SparkGameLauncher"));
    auto *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        static const QString fallbackUrl = QStringLiteral(
            "https://github.com/Open-Wine-Components/umu-launcher/releases/download/1.4.4/umu-launcher-1.4.4-zipapp.tar"
        );

        if (reply->error() != QNetworkReply::NoError) {
            // Em caso de erro na API do GitHub (ex: limite de taxa), usa o link direto
            downloadTar(fallbackUrl);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            downloadTar(fallbackUrl);
            return;
        }

        QJsonArray assets = doc.object().value(QStringLiteral("assets")).toArray();
        QString targetUrl;
        for (const QJsonValue &val : assets) {
            QJsonObject asset = val.toObject();
            QString name = asset.value(QStringLiteral("name")).toString();
            if (name.endsWith(QStringLiteral("-zipapp.tar"), Qt::CaseInsensitive) ||
                name.endsWith(QStringLiteral(".zipapp.tar"), Qt::CaseInsensitive)) {
                targetUrl = asset.value(QStringLiteral("browser_download_url")).toString();
                break;
            }
        }

        if (targetUrl.isEmpty()) {
            targetUrl = fallbackUrl;
        }

        downloadTar(targetUrl);
    });
}

void UmuManager::downloadTar(const QString &url)
{
    if (m_dialog) {
        m_dialog->setLabelText(tr("Downloading UMU Launcher..."));
    }

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SparkGameLauncher"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            int percent = static_cast<int>((received * 100) / total);
            if (m_dialog) {
                m_dialog->setValue(percent);
                m_dialog->setLabelText(tr("Downloading UMU Launcher: %1 / %2")
                    .arg(formatBytes(received), formatBytes(total)));
            }
            emit downloadProgress(received, total);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            if (m_dialog) {
                m_dialog->close();
            }
            if (m_callback) {
                m_callback(false);
            }
            emit finished(false, err);
            deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QTemporaryFile tempFile(QDir::tempPath() + QStringLiteral("/spark-umu-XXXXXX.tar"));
        tempFile.setAutoRemove(false);
        if (!tempFile.open()) {
            if (m_dialog) {
                m_dialog->close();
            }
            if (m_callback) {
                m_callback(false);
            }
            emit finished(false, tr("Failed to create temporary archive file."));
            deleteLater();
            return;
        }

        tempFile.write(data);
        tempFile.close();

        extractTar(tempFile.fileName());
    });
}

void UmuManager::extractTar(const QString &tarFilePath)
{
    if (m_dialog) {
        m_dialog->setLabelText(tr("Installing UMU Launcher..."));
    }

    QString extractDir = QDir::tempPath() + QStringLiteral("/spark-umu-extract-") + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(extractDir);

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc, extractDir, tarFilePath](int exitCode) {
        proc->deleteLater();
        QFile::remove(tarFilePath);

        if (exitCode != 0) {
            QDir(extractDir).removeRecursively();
            if (m_dialog) {
                m_dialog->close();
            }
            if (m_callback) {
                m_callback(false);
            }
            emit finished(false, tr("Failed to extract UMU Launcher archive."));
            deleteLater();
            return;
        }

        // Procura umu-run extraído
        QString destDir = QDir::homePath() + QStringLiteral("/.local/share/spark/umu");
        QDir().mkpath(destDir);
        QString destFile = destDir + QStringLiteral("/umu-run");

        QString extractedUmu = extractDir + QStringLiteral("/umu/umu-run");
        if (!QFile::exists(extractedUmu)) {
            extractedUmu = extractDir + QStringLiteral("/umu-run");
        }

        if (QFile::exists(extractedUmu)) {
            QFile::remove(destFile);
            QFile::copy(extractedUmu, destFile);
            QFile::setPermissions(destFile,
                QFile::permissions(destFile) |
                QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther |
                QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther);
        }

        QDir(extractDir).removeRecursively();

        bool success = QFile::exists(destFile) && QFileInfo(destFile).isExecutable();
        if (m_dialog) {
            m_dialog->close();
        }

        if (m_callback) {
            m_callback(success);
        }

        emit finished(success, success ? QString() : tr("umu-run binary could not be verified after installation."));
        deleteLater();
    });

    proc->setProgram(QStringLiteral("tar"));
    proc->setArguments(QStringList{QStringLiteral("-xf"), tarFilePath, QStringLiteral("-C"), extractDir});
    proc->start();
}
