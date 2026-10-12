#include "exportsettingswidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollArea>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet>
#include <QApplication>
#include <QIcon>
#include "../utils/iconhelper.h"
#include "../utils/librarymanager.h"

quint64 ExportSettingsWidget::getDirSize(const QString &dirPath)
{
    quint64 totalSize = 0;
    QDir dir(dirPath);
    if (!dir.exists()) {
        return 0;
    }

    QDirIterator it(dirPath, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }
    return totalSize;
}

QString ExportSettingsWidget::formatSize(quint64 bytes)
{
    if (bytes == 0) {
        return tr("0 KB");
    }
    const double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024.0) {
        return tr("%1 KB").arg(kb, 0, 'f', 0);
    }
    const double mb = kb / 1024.0;
    if (mb < 1024.0) {
        return tr("%1 MB").arg(mb, 0, 'f', 1);
    }
    const double gb = mb / 1024.0;
    return tr("%1 GB").arg(gb, 0, 'f', 2);
}

ExportSettingsWidget::ExportSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refreshSizes();
}

void ExportSettingsWidget::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // Cabeçalho da página
    auto *headerLabel = new QLabel(tr("<b>Export & Import Spark Data</b>"), container);
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(headerFont.pointSize() > 0 ? headerFont.pointSize() + 2 : 12);
    headerLabel->setFont(headerFont);
    mainLayout->addWidget(headerLabel);

    auto *descLabel = new QLabel(
        tr("Create compressed backups of your Spark games library, configurations, icons, and prefixes, "
           "or restore data from a previous backup archive."),
        container
    );
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // ==========================================
    // Grupo 1: Exportação de Dados
    // ==========================================
    auto *exportGroup = new QGroupBox(tr("Export Data"), container);
    auto *exportGroupLayout = new QVBoxLayout(exportGroup);
    exportGroupLayout->setSpacing(10);
    exportGroupLayout->setContentsMargins(14, 14, 14, 14);

    auto *exportInfoLabel = new QLabel(tr("Select the components you want to include in the backup file:"), exportGroup);
    exportGroupLayout->addWidget(exportInfoLabel);

    m_configCheck = new QCheckBox(tr("Configurations && Games Library (~/.config/SparkLauncher)"), exportGroup);
    m_configCheck->setChecked(true);
    exportGroupLayout->addWidget(m_configCheck);

    m_iconsCheck = new QCheckBox(tr("Game Icons (~/.local/share/spark/icons)"), exportGroup);
    m_iconsCheck->setChecked(true);
    exportGroupLayout->addWidget(m_iconsCheck);

    m_prefixesCheck = new QCheckBox(tr("Wine && Proton Prefixes (~/.local/share/spark/prefixes)"), exportGroup);
    m_prefixesCheck->setChecked(true);
    connect(m_prefixesCheck, &QCheckBox::toggled, this, &ExportSettingsWidget::onPrefixesCheckToggled);
    exportGroupLayout->addWidget(m_prefixesCheck);

    // Container de seleção individual de prefixos (indentado)
    m_prefixContainer = new QWidget(exportGroup);
    auto *prefixContainerLayout = new QVBoxLayout(m_prefixContainer);
    prefixContainerLayout->setContentsMargins(24, 0, 0, 4);
    prefixContainerLayout->setSpacing(6);

    auto *prefixHeaderLayout = new QHBoxLayout();
    prefixHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto *prefixInfo = new QLabel(tr("Select prefixes to include:"), m_prefixContainer);
    prefixInfo->setStyleSheet(QStringLiteral("color: palette(text); font-size: 11px;"));
    prefixHeaderLayout->addWidget(prefixInfo);
    prefixHeaderLayout->addStretch(1);

    m_selectAllPrefixesBtn = new QPushButton(tr("Select All"), m_prefixContainer);
    m_selectAllPrefixesBtn->setCursor(Qt::PointingHandCursor);
    connect(m_selectAllPrefixesBtn, &QPushButton::clicked, this, &ExportSettingsWidget::onSelectAllPrefixes);
    prefixHeaderLayout->addWidget(m_selectAllPrefixesBtn);

    m_deselectAllPrefixesBtn = new QPushButton(tr("Deselect All"), m_prefixContainer);
    m_deselectAllPrefixesBtn->setCursor(Qt::PointingHandCursor);
    connect(m_deselectAllPrefixesBtn, &QPushButton::clicked, this, &ExportSettingsWidget::onDeselectAllPrefixes);
    prefixHeaderLayout->addWidget(m_deselectAllPrefixesBtn);

    prefixContainerLayout->addLayout(prefixHeaderLayout);

    m_prefixListWidget = new QListWidget(m_prefixContainer);
    m_prefixListWidget->setMaximumHeight(160);
    m_prefixListWidget->setAlternatingRowColors(false);
    m_prefixListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_prefixListWidget->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "    background-color: palette(base);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 4px;"
        "    padding: 2px;"
        "}"
        "QListWidget::item {"
        "    padding: 4px 6px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: rgba(128, 128, 128, 0.15);"
        "    border-radius: 3px;"
        "}"
    ));
    connect(m_prefixListWidget, &QListWidget::itemChanged, this, &ExportSettingsWidget::onPrefixItemChanged);
    prefixContainerLayout->addWidget(m_prefixListWidget);

    exportGroupLayout->addWidget(m_prefixContainer);

    m_toolsCheck = new QCheckBox(tr("Tools && Redistributables (umu-launcher, redist)"), exportGroup);
    m_toolsCheck->setChecked(true);
    exportGroupLayout->addWidget(m_toolsCheck);

    auto *exportActionLayout = new QHBoxLayout();
    exportActionLayout->setSpacing(10);

    m_refreshBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Recalculate Sizes"), exportGroup);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ExportSettingsWidget::refreshSizes);
    exportActionLayout->addWidget(m_refreshBtn);

    exportActionLayout->addStretch(1);

    m_exportBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-save")), tr("Export Backup..."), exportGroup);
    m_exportBtn->setDefault(true);
    connect(m_exportBtn, &QPushButton::clicked, this, &ExportSettingsWidget::onExportClicked);
    exportActionLayout->addWidget(m_exportBtn);

    exportGroupLayout->addLayout(exportActionLayout);
    mainLayout->addWidget(exportGroup);

    // ==========================================
    // Grupo 2: Importação de Dados
    // ==========================================
    auto *importGroup = new QGroupBox(tr("Import Data"), container);
    auto *importGroupLayout = new QVBoxLayout(importGroup);
    importGroupLayout->setSpacing(10);
    importGroupLayout->setContentsMargins(14, 14, 14, 14);

    auto *importInfoLabel = new QLabel(
        tr("Restore your library, configurations, and files from a previously exported backup archive (.tar.gz, .tar.xz, .tar)."),
        importGroup
    );
    importInfoLabel->setWordWrap(true);
    importGroupLayout->addWidget(importInfoLabel);

    auto *warningLabel = new QLabel(
        tr("<font color='#e67e22'><b>Note:</b> Restoring will overwrite existing configurations and files with matching names.</font>"),
        importGroup
    );
    warningLabel->setWordWrap(true);
    importGroupLayout->addWidget(warningLabel);

    auto *importActionLayout = new QHBoxLayout();
    importActionLayout->addStretch(1);

    m_importBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), tr("Import Backup..."), importGroup);
    connect(m_importBtn, &QPushButton::clicked, this, &ExportSettingsWidget::onImportClicked);
    importActionLayout->addWidget(m_importBtn);

    importGroupLayout->addLayout(importActionLayout);
    mainLayout->addWidget(importGroup);

    // ==========================================
    // Informações de Pastas Locais
    // ==========================================
    auto *pathsLayout = new QHBoxLayout();
    pathsLayout->setSpacing(8);

    auto *openConfigBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), tr("Open .config Folder"), container);
    connect(openConfigBtn, &QPushButton::clicked, this, []() {
        const QString configPath = QDir::homePath() + QStringLiteral("/.config/SparkLauncher");
        QDir().mkpath(configPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(configPath));
    });
    pathsLayout->addWidget(openConfigBtn);

    auto *openDataBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), tr("Open .local/share Folder"), container);
    connect(openDataBtn, &QPushButton::clicked, this, []() {
        const QString dataPath = QDir::homePath() + QStringLiteral("/.local/share/spark");
        QDir().mkpath(dataPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dataPath));
    });
    pathsLayout->addWidget(openDataBtn);

    pathsLayout->addStretch(1);
    mainLayout->addLayout(pathsLayout);

    mainLayout->addStretch(1);

    scrollArea->setWidget(container);
    outerLayout->addWidget(scrollArea);
}

void ExportSettingsWidget::populatePrefixList()
{
    if (!m_prefixListWidget) {
        return;
    }

    // Preservar quais pastas já estavam marcadas caso o usuário clique em Recalculate
    QSet<QString> checkedFolders;
    const bool wasInitialized = (m_prefixListWidget->count() > 0);
    for (int i = 0; i < m_prefixListWidget->count(); ++i) {
        auto *it = m_prefixListWidget->item(i);
        if (it->checkState() == Qt::Checked) {
            checkedFolders.insert(it->data(Qt::UserRole).toString());
        }
    }

    m_prefixListWidget->blockSignals(true);
    m_prefixListWidget->clear();

    const QString home = QDir::homePath();
    const QString prefixesDir = home + QStringLiteral("/.local/share/spark/prefixes");
    QDir dir(prefixesDir);
    if (!dir.exists()) {
        m_prefixContainer->setVisible(false);
        m_prefixListWidget->blockSignals(false);
        return;
    }

    const QList<Game> games = LibraryManager::loadGames();
    const QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    if (subDirs.isEmpty()) {
        m_prefixContainer->setVisible(false);
        m_prefixListWidget->blockSignals(false);
        return;
    }

    for (const QFileInfo &subDir : subDirs) {
        const QString dirName = subDir.fileName();
        const QString fullPath = subDir.absoluteFilePath();
        const quint64 size = getDirSize(fullPath);

        // Associar com jogo registrado, se houver
        QString displayTitle;
        QString iconPath;
        for (const Game &g : games) {
            QFileInfo prefixFi(g.winePrefix);
            if (prefixFi.absoluteFilePath() == fullPath || prefixFi.fileName() == dirName || g.id == dirName) {
                displayTitle = g.title;
                iconPath = g.iconPath;
                break;
            }
        }

        QString itemText;
        if (!displayTitle.isEmpty()) {
            itemText = QStringLiteral("%1 (%2) — %3").arg(displayTitle, dirName, formatSize(size));
        } else {
            itemText = QStringLiteral("%1 — %2").arg(dirName, formatSize(size));
        }

        auto *item = new QListWidgetItem(m_prefixListWidget);
        item->setText(itemText);
        item->setData(Qt::UserRole, dirName);
        item->setData(Qt::UserRole + 1, static_cast<qulonglong>(size));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        const bool shouldCheck = !wasInitialized || checkedFolders.contains(dirName);
        item->setCheckState(shouldCheck ? Qt::Checked : Qt::Unchecked);

        if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
            item->setIcon(IconHelper::createGameIcon(displayTitle, iconPath, 24));
        } else {
            item->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
        }
    }

    m_prefixListWidget->blockSignals(false);
    m_prefixContainer->setVisible(m_prefixesCheck->isChecked() && !subDirs.isEmpty());
    updatePrefixSummary();
}

void ExportSettingsWidget::onPrefixesCheckToggled(bool checked)
{
    m_prefixContainer->setVisible(checked && (m_prefixListWidget->count() > 0));
}

void ExportSettingsWidget::onPrefixItemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);
    updatePrefixSummary();
}

void ExportSettingsWidget::onSelectAllPrefixes()
{
    m_prefixListWidget->blockSignals(true);
    for (int i = 0; i < m_prefixListWidget->count(); ++i) {
        m_prefixListWidget->item(i)->setCheckState(Qt::Checked);
    }
    m_prefixListWidget->blockSignals(false);
    updatePrefixSummary();
}

void ExportSettingsWidget::onDeselectAllPrefixes()
{
    m_prefixListWidget->blockSignals(true);
    for (int i = 0; i < m_prefixListWidget->count(); ++i) {
        m_prefixListWidget->item(i)->setCheckState(Qt::Unchecked);
    }
    m_prefixListWidget->blockSignals(false);
    updatePrefixSummary();
}

void ExportSettingsWidget::updatePrefixSummary()
{
    if (m_updatingPrefixSummary || !m_prefixListWidget) {
        return;
    }
    m_updatingPrefixSummary = true;

    const int totalCount = m_prefixListWidget->count();
    int checkedCount = 0;
    quint64 checkedSize = 0;

    for (int i = 0; i < totalCount; ++i) {
        auto *item = m_prefixListWidget->item(i);
        const quint64 sz = item->data(Qt::UserRole + 1).toULongLong();
        if (item->checkState() == Qt::Checked) {
            checkedCount++;
            checkedSize += sz;
        }
    }

    if (totalCount == 0) {
        m_prefixesCheck->setText(tr("Wine && Proton Prefixes (~/.local/share/spark/prefixes) — 0 KB"));
    } else if (checkedCount == totalCount) {
        m_prefixesCheck->setText(tr("Wine && Proton Prefixes (~/.local/share/spark/prefixes) — %1 (All %2 selected)")
            .arg(formatSize(checkedSize)).arg(totalCount));
    } else if (checkedCount == 0) {
        m_prefixesCheck->setText(tr("Wine && Proton Prefixes (~/.local/share/spark/prefixes) — 0 KB (None selected)"));
    } else {
        m_prefixesCheck->setText(tr("Wine && Proton Prefixes (~/.local/share/spark/prefixes) — %1 (%2 of %3 selected)")
            .arg(formatSize(checkedSize)).arg(checkedCount).arg(totalCount));
    }

    m_updatingPrefixSummary = false;
}

void ExportSettingsWidget::refreshSizes()
{
    const QString home = QDir::homePath();

    // 1. Configs
    quint64 configSize = getDirSize(home + QStringLiteral("/.config/SparkLauncher"));
    configSize += getDirSize(home + QStringLiteral("/.config/spark"));
    if (QFile::exists(home + QStringLiteral("/.config/Sparkrc"))) {
        configSize += QFileInfo(home + QStringLiteral("/.config/Sparkrc")).size();
    }
    m_configCheck->setText(tr("Configurations && Games Library (~/.config/SparkLauncher) — %1").arg(formatSize(configSize)));

    // 2. Icons
    const quint64 iconsSize = getDirSize(home + QStringLiteral("/.local/share/spark/icons"));
    m_iconsCheck->setText(tr("Game Icons (~/.local/share/spark/icons) — %1").arg(formatSize(iconsSize)));

    // 3. Prefixes (popula itens e atualiza resumo)
    populatePrefixList();

    // 4. Tools
    quint64 toolsSize = getDirSize(home + QStringLiteral("/.local/share/spark/redist"));
    toolsSize += getDirSize(home + QStringLiteral("/.local/share/spark/umu"));
    toolsSize += getDirSize(home + QStringLiteral("/.local/share/sparkle"));
    m_toolsCheck->setText(tr("Tools && Redistributables (umu, redist) — %1").arg(formatSize(toolsSize)));
}

void ExportSettingsWidget::onExportClicked()
{
    if (!m_configCheck->isChecked() &&
        !m_iconsCheck->isChecked() &&
        !m_prefixesCheck->isChecked() &&
        !m_toolsCheck->isChecked()) {
        QMessageBox::warning(this, tr("No Items Selected"), tr("Please select at least one component to export."));
        return;
    }

    const QString defaultFileName = QStringLiteral("spark_backup_%1.tar.gz")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    QString initialDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
        initialDir = QDir::homePath();
    }
    const QString initialPath = initialDir + QStringLiteral("/") + defaultFileName;

    const QString destFile = QFileDialog::getSaveFileName(
        this,
        tr("Export Spark Backup"),
        initialPath,
        tr("Compressed Archive (*.tar.gz);;Archive (*.tar);;All Files (*.*)")
    );

    if (destFile.isEmpty()) {
        return;
    }

    const QString home = QDir::homePath();
    QStringList relativePaths;

    // 1. Config
    if (m_configCheck->isChecked()) {
        if (QDir(home + QStringLiteral("/.config/SparkLauncher")).exists()) {
            relativePaths << QStringLiteral(".config/SparkLauncher");
        }
        if (QDir(home + QStringLiteral("/.config/spark")).exists()) {
            relativePaths << QStringLiteral(".config/spark");
        }
        if (QFile::exists(home + QStringLiteral("/.config/Sparkrc"))) {
            relativePaths << QStringLiteral(".config/Sparkrc");
        }
    }

    // 2. Icons
    if (m_iconsCheck->isChecked()) {
        if (QDir(home + QStringLiteral("/.local/share/spark/icons")).exists()) {
            relativePaths << QStringLiteral(".local/share/spark/icons");
        }
    }

    // 3. Prefixes
    if (m_prefixesCheck->isChecked()) {
        int selectedPrefixCount = 0;
        for (int i = 0; i < m_prefixListWidget->count(); ++i) {
            auto *item = m_prefixListWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                const QString dirName = item->data(Qt::UserRole).toString();
                const QString relPath = QStringLiteral(".local/share/spark/prefixes/") + dirName;
                if (QDir(home + QStringLiteral("/") + relPath).exists()) {
                    relativePaths << relPath;
                    selectedPrefixCount++;
                }
            }
        }
        if (m_prefixListWidget->count() > 0 && selectedPrefixCount == 0) {
            QMessageBox::warning(
                this,
                tr("No Prefixes Selected"),
                tr("You enabled 'Wine & Proton Prefixes', but no individual prefix was selected to export.\n"
                   "Please select at least one prefix or uncheck the 'Wine & Proton Prefixes' option.")
            );
            return;
        }
    }

    // 4. Tools
    if (m_toolsCheck->isChecked()) {
        if (QDir(home + QStringLiteral("/.local/share/spark/redist")).exists()) {
            relativePaths << QStringLiteral(".local/share/spark/redist");
        }
        if (QDir(home + QStringLiteral("/.local/share/spark/umu")).exists()) {
            relativePaths << QStringLiteral(".local/share/spark/umu");
        }
        if (QDir(home + QStringLiteral("/.local/share/sparkle")).exists()) {
            relativePaths << QStringLiteral(".local/share/sparkle");
        }
    }

    if (relativePaths.isEmpty()) {
        QMessageBox::warning(this, tr("Nothing to Export"), tr("The selected directories do not currently exist on disk."));
        return;
    }

    QString chosenFile = destFile;
    if (!chosenFile.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive) &&
        !chosenFile.endsWith(QStringLiteral(".tar.xz"), Qt::CaseInsensitive) &&
        !chosenFile.endsWith(QStringLiteral(".tgz"), Qt::CaseInsensitive) &&
        !chosenFile.endsWith(QStringLiteral(".tar"), Qt::CaseInsensitive)) {
        chosenFile += QStringLiteral(".tar.gz");
    }

    // Preparar comando tar
    QStringList args;
    if (chosenFile.endsWith(QStringLiteral(".tar.gz"), Qt::CaseInsensitive) ||
        chosenFile.endsWith(QStringLiteral(".tgz"), Qt::CaseInsensitive)) {
        args << QStringLiteral("-czf");
    } else if (chosenFile.endsWith(QStringLiteral(".tar.xz"), Qt::CaseInsensitive)) {
        args << QStringLiteral("-cJf");
    } else {
        args << QStringLiteral("-cf");
    }

    args << chosenFile << QStringLiteral("-C") << home;
    args.append(relativePaths);

    QProgressDialog progress(tr("Exporting Spark backup..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(200);

    QProcess process;
    process.start(QStringLiteral("tar"), args);

    connect(&progress, &QProgressDialog::canceled, [&process]() {
        if (process.state() == QProcess::Running) {
            process.kill();
        }
    });

    while (process.state() == QProcess::Running) {
        QApplication::processEvents();
        process.waitForFinished(50);
        if (progress.wasCanceled()) {
            QFile::remove(chosenFile);
            return;
        }
    }

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        QFileInfo fi(chosenFile);
        QMessageBox::information(
            this,
            tr("Export Successful"),
            tr("Spark data was successfully exported!\n\n<b>File:</b> %1\n<b>Size:</b> %2")
                .arg(fi.fileName())
                .arg(formatSize(fi.size()))
        );
    } else {
        QFile::remove(chosenFile);
        QString errorMsg = QString::fromUtf8(process.readAllStandardError());
        if (errorMsg.isEmpty()) {
            errorMsg = process.errorString();
        }
        QMessageBox::critical(
            this,
            tr("Export Failed"),
            tr("Failed to create backup archive:\n%1").arg(errorMsg)
        );
    }
}

void ExportSettingsWidget::onImportClicked()
{
    QString initialDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
        initialDir = QDir::homePath();
    }

    const QString backupFile = QFileDialog::getOpenFileName(
        this,
        tr("Select Spark Backup Archive"),
        initialDir,
        tr("Backup Archives (*.tar.gz *.tar.xz *.tar *.tgz);;All Files (*.*)")
    );

    if (backupFile.isEmpty()) {
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Import"),
        tr("Are you sure you want to import this backup?\n\n"
           "This will restore configurations and data files into your system:\n"
           "• ~/.config/SparkLauncher\n"
           "• ~/.local/share/spark\n\n"
           "Existing files with matching names will be overwritten.\n"
           "Do you wish to proceed?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    const QString home = QDir::homePath();
    const QStringList args = {
        QStringLiteral("-xf"),
        backupFile,
        QStringLiteral("-C"),
        home
    };

    QProgressDialog progress(tr("Importing Spark backup..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(200);

    QProcess process;
    process.start(QStringLiteral("tar"), args);

    connect(&progress, &QProgressDialog::canceled, [&process]() {
        if (process.state() == QProcess::Running) {
            process.kill();
        }
    });

    while (process.state() == QProcess::Running) {
        QApplication::processEvents();
        process.waitForFinished(50);
        if (progress.wasCanceled()) {
            return;
        }
    }

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        refreshSizes();
        emit dataImported();
        QMessageBox::information(
            this,
            tr("Import Successful"),
            tr("Spark backup data was successfully imported!\nYour games library and configurations have been restored.")
        );
    } else {
        QString errorMsg = QString::fromUtf8(process.readAllStandardError());
        if (errorMsg.isEmpty()) {
            errorMsg = process.errorString();
        }
        QMessageBox::critical(
            this,
            tr("Import Failed"),
            tr("Failed to extract backup archive:\n%1").arg(errorMsg)
        );
    }
}
