#include "protondownloadwidget.h"
#include "../utils/umumanager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPolygonF>

static QString formatBytes(qint64 bytes)
{
    if (bytes <= 0) {
        return QStringLiteral("0 MB");
    }
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 1024.0) {
        return QStringLiteral("%1 GB").arg(mb / 1024.0, 0, 'f', 2);
    }
    return QStringLiteral("%1 MB").arg(mb, 0, 'f', 1);
}

// -----------------------------------------------------------------------------
// ProtonRowWidget
// -----------------------------------------------------------------------------

ProtonRowWidget::ProtonRowWidget(const ProtonReleaseInfo &info, QWidget *parent)
    : QWidget(parent)
    , m_info(info)
{
    auto *rowFrame = new QFrame(this);
    rowFrame->setObjectName(QStringLiteral("protonCard"));
    rowFrame->setStyleSheet(QStringLiteral(
        "#protonCard {"
        "    background-color: palette(base);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 6px;"
        "}"
        "#protonCard:hover {"
        "    border: 1px solid rgba(61, 174, 233, 0.65);"
        "}"
    ));

    auto *frameLayout = new QHBoxLayout(rowFrame);
    frameLayout->setContentsMargins(14, 10, 14, 10);
    frameLayout->setSpacing(14);

    // Ícone da ferramenta / runner à esquerda
    m_iconLabel = new QLabel(rowFrame);
    m_iconLabel->setFixedSize(36, 36);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    frameLayout->addWidget(m_iconLabel);

    // 1. Informações da versão
    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    m_nameLabel = new QLabel(m_info.name, rowFrame);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    infoLayout->addWidget(m_nameLabel);

    QString sizeStr = m_info.size > 0 ? formatBytes(m_info.size) : tr("Proton compatibility tool");
    m_sizeLabel = new QLabel(sizeStr, rowFrame);
    m_sizeLabel->setStyleSheet(QStringLiteral("color: palette(placeholder-text); font-size: 11px;"));
    infoLayout->addWidget(m_sizeLabel);

    auto *infoContainer = new QWidget(rowFrame);
    infoContainer->setLayout(infoLayout);
    infoContainer->setFixedWidth(160);
    frameLayout->addWidget(infoContainer);

    // 2. Container central com barra de progresso (visível apenas durante download/instalação)
    auto *centerContainer = new QWidget(rowFrame);
    auto *centerLayout = new QHBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    m_progressBar = new QProgressBar(centerContainer);
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(22);
    m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar {"
        "    border: 1px solid palette(mid);"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    background: palette(alternate-base);"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3daee9;"
        "    border-radius: 3px;"
        "}"
    ));
    m_progressBar->setVisible(false);
    centerLayout->addWidget(m_progressBar);

    frameLayout->addWidget(centerContainer, 1);

    // 3. Botão de ação à direita
    m_actionButton = new QPushButton(rowFrame);
    m_actionButton->setFixedWidth(115);
    m_actionButton->setFixedHeight(30);
    m_actionButton->setCursor(Qt::PointingHandCursor);
    frameLayout->addWidget(m_actionButton);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 2, 0, 2);
    outerLayout->addWidget(rowFrame);

    connect(m_actionButton, &QPushButton::clicked, this, [this]() {
        if (m_isDownloading) {
            emit cancelRequested();
        } else {
            emit downloadRequested(m_info);
        }
    });

    setInstalled(m_info.isInstalled);
}

void ProtonRowWidget::updateIcon()
{
    const int iconBox = 36;
    QPixmap baseIcon(iconBox, iconBox);
    baseIcon.fill(Qt::transparent);

    QPainter p(&baseIcon);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0, 0, QIcon::fromTheme(
        QStringLiteral("wine"),
        QIcon::fromTheme(QStringLiteral("applications-system"),
        QIcon::fromTheme(QStringLiteral("application-x-executable"))
    )).pixmap(32, 32));

    if (m_info.isInstalled) {
        // Ícone V verde de check no canto inferior direito do ícone do Proton
        const int badgeSize = 16;
        QPixmap badge(badgeSize, badgeSize);
        badge.fill(Qt::transparent);
        QPainter bp(&badge);
        bp.setRenderHint(QPainter::Antialiasing);

        bp.setPen(QPen(QColor(15, 18, 24, 220), 1.5));
        bp.setBrush(QColor(QStringLiteral("#27ae60")));
        bp.drawEllipse(1, 1, badgeSize - 2, badgeSize - 2);

        bp.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF check;
        check << QPointF(badgeSize * 0.26, badgeSize * 0.52)
              << QPointF(badgeSize * 0.44, badgeSize * 0.70)
              << QPointF(badgeSize * 0.74, badgeSize * 0.32);
        bp.drawPolyline(check);
        bp.end();

        p.drawPixmap(iconBox - badgeSize, iconBox - badgeSize, badge);
    }
    p.end();

    m_iconLabel->setPixmap(baseIcon);
}

void ProtonRowWidget::setInstalled(bool installed)
{
    m_info.isInstalled = installed;
    m_isDownloading = false;

    updateIcon();

    // A barra de progresso fica oculta quando não estiver instalando
    m_progressBar->setVisible(false);

    if (installed) {
        m_actionButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
        m_actionButton->setText(tr("Reinstall"));
        m_actionButton->setEnabled(true);
    } else {
        m_actionButton->setIcon(QIcon::fromTheme(QStringLiteral("download"),
                                                 QIcon::fromTheme(QStringLiteral("edit-download"))));
        m_actionButton->setText(tr("Download"));
        m_actionButton->setEnabled(true);
    }
}

void ProtonRowWidget::setDownloading(bool downloading)
{
    m_isDownloading = downloading;
    if (downloading) {
        m_progressBar->setVisible(true);
        m_progressBar->setValue(0);
        m_progressBar->setFormat(tr("Connecting..."));
        m_actionButton->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
        m_actionButton->setText(tr("Cancel"));
        m_actionButton->setEnabled(true);
    } else {
        setInstalled(m_info.isInstalled);
    }
}

void ProtonRowWidget::updateProgress(qint64 received, qint64 total)
{
    m_progressBar->setVisible(true);
    if (total > 0) {
        int percent = static_cast<int>((received * 100) / total);
        m_progressBar->setValue(percent);
        m_progressBar->setFormat(QStringLiteral("%1% (%2 / %3)")
            .arg(percent)
            .arg(formatBytes(received))
            .arg(formatBytes(total)));
    } else {
        m_progressBar->setValue(0);
        m_progressBar->setFormat(formatBytes(received));
    }
}

void ProtonRowWidget::setExtracting()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(100);
    m_progressBar->setFormat(tr("Extracting..."));
    m_actionButton->setEnabled(false);
}

void ProtonRowWidget::setError(const QString &msg)
{
    m_isDownloading = false;
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(tr("Failed: %1").arg(msg));
    m_actionButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_actionButton->setText(tr("Retry"));
    m_actionButton->setEnabled(true);
}

// -----------------------------------------------------------------------------
// ProtonDownloadWidget
// -----------------------------------------------------------------------------

ProtonDownloadWidget::ProtonDownloadWidget(QWidget *parent)
    : QWidget(parent)
    , m_netManager(new QNetworkAccessManager(this))
{
    setupUi();
    populateRows(getFallbackReleases());
    refreshReleases();
}

ProtonDownloadWidget::~ProtonDownloadWidget()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
    }
    if (m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->close();
        if (!m_downloadTempPath.isEmpty()) {
            QFile::remove(m_downloadTempPath);
        }
    }
}

void ProtonDownloadWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // Cabeçalho da página
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *titleLayout = new QVBoxLayout();
    auto *titleLabel = new QLabel(tr("Proton Downloads"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 3);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);

    m_statusLabel = new QLabel(tr("Available GE-Proton releases from GloriousEggroll"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: palette(placeholder-text); font-size: 12px;"));
    titleLayout->addWidget(m_statusLabel);

    headerLayout->addLayout(titleLayout, 1);

    m_refreshButton = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Check Updates"), this);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProtonDownloadWidget::refreshReleases);
    headerLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(headerLayout);

    // Separador
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // UMU Launcher Card
    m_umuCard = new QFrame(this);
    m_umuCard->setObjectName(QStringLiteral("umuCard"));
    m_umuCard->setStyleSheet(QStringLiteral(
        "#umuCard {"
        "    background-color: palette(base);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 6px;"
        "}"
    ));
    auto *umuLayout = new QHBoxLayout(m_umuCard);
    umuLayout->setContentsMargins(14, 10, 14, 10);
    umuLayout->setSpacing(14);

    auto *umuIconLabel = new QLabel(m_umuCard);
    umuIconLabel->setFixedSize(36, 36);
    umuIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("applications-games")).pixmap(32, 32));
    umuLayout->addWidget(umuIconLabel);

    auto *umuInfoLayout = new QVBoxLayout();
    auto *umuTitle = new QLabel(QStringLiteral("<b>UMU Launcher</b> (Steam Linux Runtime Compatibility)"), m_umuCard);
    umuInfoLayout->addWidget(umuTitle);

    m_umuStatusLabel = new QLabel(m_umuCard);
    m_umuStatusLabel->setStyleSheet(QStringLiteral("color: palette(placeholder-text); font-size: 11px;"));
    umuInfoLayout->addWidget(m_umuStatusLabel);
    umuLayout->addLayout(umuInfoLayout, 1);

    m_umuButton = new QPushButton(m_umuCard);
    m_umuButton->setCursor(Qt::PointingHandCursor);
    connect(m_umuButton, &QPushButton::clicked, this, [this]() {
        auto *mgr = new UmuManager(this);
        mgr->startDownload(this, [this](bool success) {
            Q_UNUSED(success);
            updateUmuCard();
        });
    });
    umuLayout->addWidget(m_umuButton);

    mainLayout->addWidget(m_umuCard);
    updateUmuCard();

    // Scroll Area com a lista de versões
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    m_rowsLayout = new QVBoxLayout(scrollContent);
    m_rowsLayout->setContentsMargins(0, 4, 6, 4);
    m_rowsLayout->setSpacing(10);
    m_rowsLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
}

void ProtonDownloadWidget::updateUmuCard()
{
    QString bin = UmuManager::findUmuBin();
    if (!bin.isEmpty()) {
        m_umuStatusLabel->setText(tr("Installed: %1").arg(bin));
        m_umuButton->setText(tr("Reinstall / Update"));
        m_umuButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    } else {
        m_umuStatusLabel->setText(tr("Not installed. Recommended for running games with Proton."));
        m_umuButton->setText(tr("Download UMU"));
        m_umuButton->setIcon(QIcon::fromTheme(QStringLiteral("download")));
    }
}

QString ProtonDownloadWidget::getSteamToolsDir() const
{
    QString dir = QDir::homePath() + QStringLiteral("/.local/share/Steam/compatibilitytools.d");
    if (QDir(dir).exists()) {
        return dir;
    }
    QString altDir = QDir::homePath() + QStringLiteral("/.steam/root/compatibilitytools.d");
    if (QDir(altDir).exists()) {
        return altDir;
    }
    return dir;
}

bool ProtonDownloadWidget::checkIfInstalled(const QString &name, const QString &tagName) const
{
    QStringList searchDirs = {
        QDir::homePath() + QStringLiteral("/.local/share/Steam/compatibilitytools.d"),
        QDir::homePath() + QStringLiteral("/.steam/root/compatibilitytools.d"),
        QDir::homePath() + QStringLiteral("/.steam/steam/compatibilitytools.d")
    };

    QStringList variants = {
        name,
        tagName,
        tagName + QStringLiteral("-x86_64"),
        name + QStringLiteral("-x86_64")
    };
    variants.removeDuplicates();

    for (const QString &dirPath : searchDirs) {
        for (const QString &variant : variants) {
            QDir d(dirPath + QLatin1Char('/') + variant);
            if (d.exists() && (QFile::exists(d.filePath(QStringLiteral("proton"))) ||
                               QFile::exists(d.filePath(QStringLiteral("compatibilitytool.vdf"))))) {
                return true;
            }
        }
    }
    return false;
}

QList<ProtonReleaseInfo> ProtonDownloadWidget::getFallbackReleases() const
{
    QList<ProtonReleaseInfo> list;

    auto add = [&](const QString &tag, qint64 size, const QString &asset) {
        ProtonReleaseInfo info;
        info.name = tag;
        info.tagName = tag;
        info.size = size;
        info.downloadUrl = QStringLiteral("https://github.com/GloriousEggroll/proton-ge-custom/releases/download/%1/%2")
                               .arg(tag, asset);
        info.isInstalled = checkIfInstalled(tag, tag);
        list.append(info);
    };

    add(QStringLiteral("GE-Proton11-7"), 563784602, QStringLiteral("GE-Proton11-7-x86_64.tar.gz"));
    add(QStringLiteral("GE-Proton11-6"), 533700853, QStringLiteral("GE-Proton11-6-x86_64.tar.gz"));
    add(QStringLiteral("GE-Proton11-5"), 533066604, QStringLiteral("GE-Proton11-5-x86_64.tar.gz"));
    add(QStringLiteral("GE-Proton9-25"), 482596482, QStringLiteral("GE-Proton9-25.tar.gz"));
    add(QStringLiteral("GE-Proton9-24"), 482103401, QStringLiteral("GE-Proton9-24.tar.gz"));
    add(QStringLiteral("GE-Proton9-20"), 480112440, QStringLiteral("GE-Proton9-20.tar.gz"));
    add(QStringLiteral("GE-Proton8-32"), 450419200, QStringLiteral("GE-Proton8-32.tar.gz"));

    return list;
}

void ProtonDownloadWidget::populateRows(const QList<ProtonReleaseInfo> &releases)
{
    // Limpa widgets antigos mantendo o stretch final
    while (m_rowsLayout->count() > 1) {
        QLayoutItem *item = m_rowsLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    for (const auto &info : releases) {
        auto *row = new ProtonRowWidget(info, this);
        connect(row, &ProtonRowWidget::downloadRequested, this, &ProtonDownloadWidget::startDownload);
        connect(row, &ProtonRowWidget::cancelRequested, this, &ProtonDownloadWidget::cancelCurrentDownload);
        m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);
    }
}

void ProtonDownloadWidget::refreshReleases()
{
    if (m_releasesReply) {
        return;
    }

    m_refreshButton->setEnabled(false);
    m_statusLabel->setText(tr("Fetching releases from GitHub..."));

    QUrl url(QStringLiteral("https://api.github.com/repos/GloriousEggroll/proton-ge-custom/releases?per_page=12"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Spark-Game-Launcher"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_releasesReply = m_netManager->get(request);
    connect(m_releasesReply, &QNetworkReply::finished, this, &ProtonDownloadWidget::onReleasesReplyFinished);
}

void ProtonDownloadWidget::onReleasesReplyFinished()
{
    m_refreshButton->setEnabled(true);
    if (!m_releasesReply) {
        return;
    }

    if (m_releasesReply->error() == QNetworkReply::NoError) {
        QByteArray data = m_releasesReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QList<ProtonReleaseInfo> fetchedList;
            const QJsonArray releasesArray = doc.array();

            for (const QJsonValue &relVal : releasesArray) {
                QJsonObject relObj = relVal.toObject();
                QString tag = relObj.value(QStringLiteral("tag_name")).toString();
                if (tag.isEmpty()) {
                    continue;
                }

                QJsonArray assets = relObj.value(QStringLiteral("assets")).toArray();
                QString downloadUrl;
                qint64 assetSize = 0;

                for (const QJsonValue &assetVal : assets) {
                    QJsonObject assetObj = assetVal.toObject();
                    QString name = assetObj.value(QStringLiteral("name")).toString();
                    if (name.endsWith(QStringLiteral(".tar.gz")) && !name.contains(QStringLiteral("aarch64"))) {
                        downloadUrl = assetObj.value(QStringLiteral("browser_download_url")).toString();
                        assetSize = static_cast<qint64>(assetObj.value(QStringLiteral("size")).toDouble());
                        break;
                    }
                }

                if (!downloadUrl.isEmpty()) {
                    ProtonReleaseInfo info;
                    info.name = tag;
                    info.tagName = tag;
                    info.downloadUrl = downloadUrl;
                    info.size = assetSize;
                    info.isInstalled = checkIfInstalled(tag, tag);
                    fetchedList.append(info);
                }
            }

            if (!fetchedList.isEmpty()) {
                populateRows(fetchedList);
                m_statusLabel->setText(tr("Releases updated successfully (%1 versions available)").arg(fetchedList.size()));
            }
        }
    } else {
        m_statusLabel->setText(tr("Using cached releases (GitHub check: %1)").arg(m_releasesReply->errorString()));
    }

    m_releasesReply->deleteLater();
    m_releasesReply = nullptr;
}

void ProtonDownloadWidget::startDownload(const ProtonReleaseInfo &info)
{
    if (m_downloadReply) {
        QMessageBox::information(this, tr("Download in progress"),
                                 tr("A Proton version is already downloading. Please wait or cancel it first."));
        return;
    }

    // Localiza a linha correspondente
    m_activeRow = nullptr;
    for (int i = 0; i < m_rowsLayout->count() - 1; ++i) {
        auto *row = qobject_cast<ProtonRowWidget*>(m_rowsLayout->itemAt(i)->widget());
        if (row && row->info().tagName == info.tagName) {
            m_activeRow = row;
            break;
        }
    }

    if (!m_activeRow) {
        return;
    }

    QString tempDir = QDir::tempPath() + QStringLiteral("/spark_downloads");
    QDir().mkpath(tempDir);

    QString fileName = QFileInfo(QUrl(info.downloadUrl).path()).fileName();
    if (fileName.isEmpty()) {
        fileName = info.tagName + QStringLiteral(".tar.gz");
    }
    m_downloadTempPath = tempDir + QLatin1Char('/') + fileName;

    m_downloadFile = std::make_unique<QFile>(m_downloadTempPath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        m_activeRow->setError(tr("Cannot create temp file"));
        m_activeRow = nullptr;
        return;
    }

    m_activeDownloadInfo = info;
    m_activeRow->setDownloading(true);
    m_statusLabel->setText(tr("Downloading %1...").arg(info.name));

    QNetworkRequest request(QUrl(info.downloadUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Spark-Game-Launcher"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_netManager->get(request);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &ProtonDownloadWidget::onDownloadProgress);
    connect(m_downloadReply, &QIODevice::readyRead, this, &ProtonDownloadWidget::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::finished, this, &ProtonDownloadWidget::onDownloadFinished);
}

void ProtonDownloadWidget::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_activeRow) {
        m_activeRow->updateProgress(bytesReceived, bytesTotal);
    }
}

void ProtonDownloadWidget::onDownloadReadyRead()
{
    if (m_downloadReply && m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->write(m_downloadReply->readAll());
    }
}

void ProtonDownloadWidget::cancelCurrentDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
    }
    if (m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->close();
        QFile::remove(m_downloadTempPath);
    }
    if (m_activeRow) {
        m_activeRow->setDownloading(false);
        m_activeRow = nullptr;
    }
    m_statusLabel->setText(tr("Download cancelled"));
}

void ProtonDownloadWidget::onDownloadFinished()
{
    if (!m_downloadReply) {
        return;
    }

    if (m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->write(m_downloadReply->readAll());
        m_downloadFile->flush();
        m_downloadFile->close();
    }

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        if (m_downloadReply->error() != QNetworkReply::OperationCanceledError) {
            if (m_activeRow) {
                m_activeRow->setError(m_downloadReply->errorString());
            }
            m_statusLabel->setText(tr("Download error: %1").arg(m_downloadReply->errorString()));
        }
        QFile::remove(m_downloadTempPath);
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
        m_activeRow = nullptr;
        return;
    }

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;

    // Extração para ~/.local/share/Steam/compatibilitytools.d
    if (m_activeRow) {
        m_activeRow->setExtracting();
    }
    m_statusLabel->setText(tr("Extracting %1 into compatibilitytools.d...").arg(m_activeDownloadInfo.name));

    QString toolsDir = getSteamToolsDir();
    QDir().mkpath(toolsDir);

    auto *process = new QProcess(this);
    QString tempPath = m_downloadTempPath;
    ProtonRowWidget *activeRow = m_activeRow;

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, tempPath, activeRow](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        QFile::remove(tempPath);

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            if (activeRow) {
                activeRow->setInstalled(true);
            }
            m_statusLabel->setText(tr("%1 installed successfully!").arg(m_activeDownloadInfo.name));
        } else {
            if (activeRow) {
                activeRow->setError(tr("Extraction failed"));
            }
            m_statusLabel->setText(tr("Extraction failed with exit code %1").arg(exitCode));
        }
        m_activeRow = nullptr;
    });

    process->start(QStringLiteral("tar"), {
        QStringLiteral("-xzf"),
        m_downloadTempPath,
        QStringLiteral("-C"),
        toolsDir
    });
}
