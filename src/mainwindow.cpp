#include "mainwindow.h"
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QPixmapCache>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QCheckBox>
#include <QWidget>
#include <QApplication>
#include <QStyleHints>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>

#include "dialogs/gameconfigdialog.h"
#include "dialogs/logviewerdialog.h"
#include "dialogs/settingsdialog.h"
#include "utils/iconhelper.h"
#include "utils/librarymanager.h"
#include "utils/processmanager.h"
#include "utils/umumanager.h"
#include "utils/configmanager.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Spark"));
    setWindowIcon(QIcon(QStringLiteral(":/logo.png")));
    resize(900, 550);

    m_processManager = new ProcessManager(this);
    connect(m_processManager, &ProcessManager::gameStarted, this, &MainWindow::onGameStarted);
    connect(m_processManager, &ProcessManager::gameFinished, this, &MainWindow::onGameFinished);
    connect(m_processManager, &ProcessManager::gameError, this, &MainWindow::onGameError);

    setupCentralWidget();
    setupToolBar();
    loadLibrary();
    updateToolbarState();

    if (qApp && qApp->styleHints()) {
        connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
            QTimer::singleShot(20, this, &MainWindow::applyThemeChange);
        });
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(tr("Main Toolbar"));
    m_toolBar->setObjectName(QStringLiteral("mainToolBar"));
    m_toolBar->setMovable(false);
    m_toolBar->setFloatable(false);
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setIconSize(QSize(22, 22));

    m_addGameAction = new QAction(tr("Add"), this);
    m_addGameAction->setToolTip(tr("Add new game"));
    connect(m_addGameAction, &QAction::triggered, this, &MainWindow::onAddGameClicked);

    m_editGameAction = new QAction(tr("Edit"), this);
    m_editGameAction->setToolTip(tr("Edit selected game"));
    connect(m_editGameAction, &QAction::triggered, this, &MainWindow::onEditGameClicked);

    m_removeGameAction = new QAction(tr("Delete"), this);
    m_removeGameAction->setToolTip(tr("Delete selected game"));
    connect(m_removeGameAction, &QAction::triggered, this, &MainWindow::onRemoveGameClicked);

    m_playPauseAction = new QAction(tr("Play"), this);
    m_playPauseAction->setToolTip(tr("Start or stop selected game"));
    connect(m_playPauseAction, &QAction::triggered, this, &MainWindow::onPlayPauseClicked);

    m_settingsAction = new QAction(tr("Settings"), this);
    m_settingsAction->setToolTip(tr("Configure Spark"));
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);

    syncIconTheme();
    updateToolBarIcons();

    m_toolBar->addAction(m_addGameAction);
    m_toolBar->addAction(m_editGameAction);
    m_toolBar->addAction(m_removeGameAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_playPauseAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_settingsAction);
}

void MainWindow::setupCentralWidget()
{
    m_gameListWidget = new QListWidget(this);
    m_gameListWidget->setObjectName(QStringLiteral("gameListWidget"));
    m_gameListWidget->setViewMode(QListView::IconMode);
    m_gameListWidget->setMovement(QListView::Static);
    m_gameListWidget->setResizeMode(QListView::Adjust);
    m_gameListWidget->setSpacing(14);
    m_gameListWidget->setIconSize(QSize(80, 80));
    m_gameListWidget->setGridSize(QSize(110, 128));
    m_gameListWidget->setWordWrap(true);
    m_gameListWidget->setFrameShape(QFrame::NoFrame);
    m_gameListWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    QFont listFont = m_gameListWidget->font();
    if (listFont.pointSizeF() > 0.0) {
        listFont.setPointSizeF(listFont.pointSizeF() - 1.0);
    } else if (listFont.pixelSize() > 0) {
        listFont.setPixelSize(listFont.pixelSize() - 2);
    }
    m_gameListWidget->setFont(listFont);

    // Habilitar rastreamento do cursor para pointer nos itens
    m_gameListWidget->setMouseTracking(true);
    m_gameListWidget->viewport()->setMouseTracking(true);
    m_gameListWidget->viewport()->installEventFilter(this);

    updateGameListStyle();

    connect(m_gameListWidget, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onContextMenuRequested);
    connect(m_gameListWidget, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onItemSelectionChanged);
    connect(m_gameListWidget, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onItemDoubleClicked);

    setCentralWidget(m_gameListWidget);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_gameListWidget && watched == m_gameListWidget->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            QListWidgetItem *item = m_gameListWidget->itemAt(mouseEvent->pos());
            if (item) {
                m_gameListWidget->viewport()->setCursor(Qt::PointingHandCursor);
            } else {
                m_gameListWidget->viewport()->unsetCursor();
            }
        } else if (event->type() == QEvent::Leave) {
            m_gameListWidget->viewport()->unsetCursor();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

QMenu *MainWindow::createPopupMenu()
{
    // Impede o menu de contexto padrão do QMainWindow que permite ocultar a toolbar
    return nullptr;
}

void MainWindow::loadLibrary()
{
    m_games = LibraryManager::loadGames();
    for (const Game &game : m_games) {
        addGameToGrid(game);
    }
}

void MainWindow::addGameToGrid(const Game &game)
{
    auto *item = new QListWidgetItem(m_gameListWidget);
    item->setText(game.title);

    QFont font = m_gameListWidget->font();
    font.setBold(true);
    item->setFont(font);

    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
    bool isRunning = m_processManager->isGameRunning(game.id);
    item->setIcon(IconHelper::createGameIcon(game.title, game.iconPath, 128, isRunning));
    item->setData(Qt::UserRole, game.id);
    item->setToolTip(game.executablePath);
}

void MainWindow::updateGameInGrid(const Game &game)
{
    for (int i = 0; i < m_gameListWidget->count(); ++i) {
        QListWidgetItem *item = m_gameListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == game.id) {
            bool isRunning = m_processManager->isGameRunning(game.id);
            item->setText(game.title);
            item->setIcon(IconHelper::createGameIcon(game.title, game.iconPath, 128, isRunning));
            item->setToolTip(game.executablePath);
            break;
        }
    }
}

QString MainWindow::selectedGameId() const
{
    if (!m_gameListWidget) {
        return QString();
    }
    QListWidgetItem *item = m_gameListWidget->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

const Game* MainWindow::findGame(const QString &gameId) const
{
    for (const Game &g : m_games) {
        if (g.id == gameId) {
            return &g;
        }
    }
    return nullptr;
}

void MainWindow::updateToolbarState()
{
    if (!m_editGameAction || !m_removeGameAction || !m_playPauseAction) {
        return;
    }

    QString gameId = selectedGameId();
    if (gameId.isEmpty()) {
        m_editGameAction->setEnabled(false);
        m_removeGameAction->setEnabled(false);
        m_playPauseAction->setEnabled(false);
        m_playPauseAction->setText(tr("Play"));
        m_playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        m_playPauseAction->setToolTip(tr("Select a game to play"));
        return;
    }

    const Game *game = findGame(gameId);
    bool isRunning = m_processManager->isGameRunning(gameId);

    m_editGameAction->setEnabled(!isRunning);
    m_removeGameAction->setEnabled(!isRunning);
    m_playPauseAction->setEnabled(true);
    if (isRunning) {
        m_playPauseAction->setText(tr("Stop"));
        m_playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
        m_playPauseAction->setToolTip(tr("Stop %1").arg(game ? game->title : QString()));
    } else {
        m_playPauseAction->setText(tr("Play"));
        m_playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        m_playPauseAction->setToolTip(tr("Play %1").arg(game ? game->title : QString()));
    }
}

void MainWindow::updateToolBarIcons()
{
    if (m_addGameAction) {
        m_addGameAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    }
    if (m_editGameAction) {
        m_editGameAction->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
    }
    if (m_removeGameAction) {
        const QStringList trashIconCandidates = {
            QStringLiteral("trash-empty-symbolic"),
            QStringLiteral("user-trash-symbolic"),
            QStringLiteral("edit-delete-symbolic")
        };
        QIcon trashIcon;
        for (const QString &name : trashIconCandidates) {
            if (QIcon::hasThemeIcon(name)) {
                trashIcon = QIcon::fromTheme(name);
                break;
            }
        }
        if (trashIcon.isNull()) {
            trashIcon = QIcon::fromTheme(QStringLiteral("user-trash"),
                        QIcon::fromTheme(QStringLiteral("edit-delete")));
        }
        m_removeGameAction->setIcon(trashIcon);
    }
    if (m_settingsAction) {
        m_settingsAction->setIcon(QIcon::fromTheme(QStringLiteral("settings-configure"),
            QIcon::fromTheme(QStringLiteral("configure"),
            QIcon::fromTheme(QStringLiteral("preferences-system")))));
    }
    updateToolbarState();
}

void MainWindow::syncIconTheme()
{
    const QColor windowColor = QApplication::palette().color(QPalette::Window);
    const bool isDark = (windowColor.lightness() < 128);

    const QString currentTheme = QIcon::themeName();
    QString targetTheme = currentTheme;

    if (isDark) {
        if (!currentTheme.contains(QStringLiteral("dark"), Qt::CaseInsensitive)) {
            if (QFile::exists(QStringLiteral("/usr/share/icons/") + currentTheme + QStringLiteral("-dark"))) {
                targetTheme = currentTheme + QStringLiteral("-dark");
            } else if (QFile::exists(QStringLiteral("/usr/share/icons/breeze-dark"))) {
                targetTheme = QStringLiteral("breeze-dark");
            }
        }
    } else {
        if (currentTheme.contains(QStringLiteral("dark"), Qt::CaseInsensitive)) {
            QString lightCandidate = currentTheme;
            lightCandidate.remove(QStringLiteral("-dark"), Qt::CaseInsensitive);
            lightCandidate.remove(QStringLiteral("dark"), Qt::CaseInsensitive);
            if (QFile::exists(QStringLiteral("/usr/share/icons/") + lightCandidate)) {
                targetTheme = lightCandidate;
            } else if (QFile::exists(QStringLiteral("/usr/share/icons/breeze"))) {
                targetTheme = QStringLiteral("breeze");
            }
        }
    }

    if (!targetTheme.isEmpty() && targetTheme != currentTheme) {
        QIcon::setThemeName(targetTheme);
    }
}

void MainWindow::updateGameListStyle()
{
    if (!m_gameListWidget) {
        return;
    }

    m_gameListWidget->setPalette(QApplication::palette());
    m_gameListWidget->viewport()->setPalette(QApplication::palette());

    m_gameListWidget->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "    background: transparent;"
        "    outline: 0;"
        "    padding: 10px;"
        "}"
        "QListWidget::item {"
        "    color: palette(text);"
        "    border: 1px solid transparent;"
        "    border-radius: 5px;"
        "    padding: 4px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: rgba(61, 174, 233, 0.18);"
        "    border: 1px solid rgba(61, 174, 233, 0.55);"
        "}"
        "QListWidget::item:selected {"
        "    background-color: rgba(61, 174, 233, 0.32);"
        "    border: 1px solid rgba(61, 174, 233, 0.85);"
        "}"
        "QListWidget::item:selected:hover {"
        "    background-color: rgba(61, 174, 233, 0.42);"
        "    border: 1px solid rgba(61, 174, 233, 0.95);"
        "}"
    ));

    m_gameListWidget->style()->unpolish(m_gameListWidget);
    m_gameListWidget->style()->polish(m_gameListWidget);
    m_gameListWidget->viewport()->update();
}

void MainWindow::applyThemeChange()
{
    static bool s_updatingTheme = false;
    if (s_updatingTheme) {
        return;
    }
    s_updatingTheme = true;

    syncIconTheme();
    QPixmapCache::clear();
    updateToolBarIcons();
    updateGameListStyle();
    update();

    s_updatingTheme = false;
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        applyThemeChange();
        QTimer::singleShot(0, this, &MainWindow::applyThemeChange);
    } else if (event->type() == QEvent::ThemeChange) {
        QTimer::singleShot(20, this, &MainWindow::applyThemeChange);
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::onEditGameClicked()
{
    QString gameId = selectedGameId();
    if (!gameId.isEmpty()) {
        onEditGame(gameId);
    }
}

void MainWindow::onRemoveGameClicked()
{
    QString gameId = selectedGameId();
    if (!gameId.isEmpty()) {
        onRemoveGame(gameId);
    }
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::libraryChanged, this, [this]() {
        m_gameListWidget->clear();
        loadLibrary();
        updateToolbarState();
    });
    dialog.exec();
    if (dialog.isLibraryModified()) {
        m_gameListWidget->clear();
        loadLibrary();
        updateToolbarState();
    }
}

void MainWindow::toggleGame(const QString &gameId)
{
    const Game *game = findGame(gameId);
    if (!game) {
        return;
    }

    if (m_processManager->isGameRunning(gameId)) {
        m_processManager->stopGame(gameId);
    } else {
        launchGame(gameId, false);
    }
}

void MainWindow::launchGame(const QString &gameId, bool closeOnLaunch)
{
    const Game *game = findGame(gameId);
    if (!game) {
        return;
    }

    bool isProton = game->runner == GameRunner::Wine &&
                    game->wineVersion.contains(QStringLiteral("proton"), Qt::CaseInsensitive);

    if (isProton && !UmuManager::isUmuAvailable()) {
        UmuManager::ensureUmuInstalled(this, [this, gameId, closeOnLaunch](bool success) {
            Q_UNUSED(success);
            const Game *g = findGame(gameId);
            if (!g) {
                return;
            }
            if (m_processManager->startGame(*g, closeOnLaunch)) {
                if (closeOnLaunch) {
                    close();
                }
            }
        });
        return;
    }

    if (m_processManager->startGame(*game, closeOnLaunch)) {
        if (closeOnLaunch) {
            close();
        }
    }
}

void MainWindow::onPlayPauseClicked()
{
    QString gameId = selectedGameId();
    if (!gameId.isEmpty()) {
        toggleGame(gameId);
    }
}

void MainWindow::onItemSelectionChanged()
{
    updateToolbarState();
}

void MainWindow::onItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        toggleGame(item->data(Qt::UserRole).toString());
    }
}

void MainWindow::onAddGameClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select Game Executable"),
        QDir::homePath(),
        tr("Executables (*.exe *.sh *.bin *.x86_64);;All Files (*)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(filePath);

    Game initialGame;
    initialGame.title = fileInfo.completeBaseName();
    initialGame.executablePath = filePath;

    if (filePath.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        initialGame.runner = GameRunner::Wine;
        initialGame.wineVersion = ConfigManager::defaultWineVersion();
        initialGame.winePrefix = ConfigManager::defaultWinePrefix();
        if (IconHelper::isIcoutilsAvailable()) {
            initialGame.iconPath = IconHelper::extractExeIcon(filePath);
        }
    } else {
        initialGame.runner = GameRunner::Native;
    }

    GameConfigDialog dialog(initialGame, this);
    dialog.setWindowTitle(tr("Add Game"));

    if (dialog.exec() == QDialog::Accepted) {
        Game configuredGame = dialog.game();
        m_games.append(configuredGame);
        addGameToGrid(configuredGame);
        LibraryManager::saveGames(m_games);
        updateToolbarState();
    }
}

void MainWindow::onContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = m_gameListWidget->itemAt(pos);
    if (!item) {
        return;
    }

    QString gameId = item->data(Qt::UserRole).toString();
    const Game *game = findGame(gameId);
    bool isRunning = m_processManager->isGameRunning(gameId);

    QMenu menu(this);
    QAction *playStopAction = nullptr;
    QAction *playAndCloseAction = nullptr;
    if (isRunning) {
        playStopAction = menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), tr("Stop"));
    } else {
        playStopAction = menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), tr("Play"));
        playAndCloseAction = menu.addAction(tr("Play and Close"));
    }

    QAction *winetricksAction = nullptr;
    QAction *runExeAction = nullptr;
    if (game && game->runner == GameRunner::Wine) {
        menu.addSeparator();
        winetricksAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("wine-symbolic"),
                             QIcon::fromTheme(QStringLiteral("wine"),
                             QIcon::fromTheme(QStringLiteral("applications-system")))),
            tr("Open Winetricks")
        );
        runExeAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("application-x-executable"), QIcon::fromTheme(QStringLiteral("system-run"))),
            tr("Run EXE in Prefix...")
        );
    }

    menu.addSeparator();
    QAction *viewLogsAction = menu.addAction(
        QIcon::fromTheme(QStringLiteral("text-x-log"), QIcon::fromTheme(QStringLiteral("utilities-terminal"))),
        tr("View Logs")
    );

    menu.addSeparator();
    QAction *editAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-edit")), tr("Edit"));
    QAction *removeAction = menu.addAction(m_removeGameAction->icon(), tr("Remove"));

    if (isRunning) {
        removeAction->setEnabled(false);
        removeAction->setToolTip(tr("Cannot remove a game while it is running"));
    }

    QAction *selectedAction = menu.exec(m_gameListWidget->viewport()->mapToGlobal(pos));
    if (!selectedAction) {
        return;
    }

    if (playStopAction && selectedAction == playStopAction) {
        toggleGame(gameId);
    } else if (playAndCloseAction && selectedAction == playAndCloseAction) {
        launchGame(gameId, true);
    } else if (winetricksAction && selectedAction == winetricksAction) {
        onOpenWinetricks(gameId);
    } else if (runExeAction && selectedAction == runExeAction) {
        onRunExeInPrefix(gameId);
    } else if (viewLogsAction && selectedAction == viewLogsAction) {
        onViewLogs(gameId);
    } else if (editAction && selectedAction == editAction) {
        onEditGame(gameId);
    } else if (removeAction && selectedAction == removeAction) {
        onRemoveGame(gameId);
    }
}

void MainWindow::onViewLogs(const QString &gameId)
{
    const Game *game = findGame(gameId);
    if (!game) {
        return;
    }

    auto *dialog = new LogViewerDialog(game->id, game->title, m_processManager, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::onOpenWinetricks(const QString &gameId)
{
    const Game *game = findGame(gameId);
    if (!game) {
        return;
    }

    QString errorMessage;
    if (!m_processManager->openWinetricks(*game, &errorMessage)) {
        QMessageBox::warning(this, tr("Winetricks"), errorMessage);
    }
}

void MainWindow::onRunExeInPrefix(const QString &gameId)
{
    const Game *game = findGame(gameId);
    if (!game) {
        return;
    }

    QString startDir = QFileInfo(game->executablePath).dir().absolutePath();
    if (!QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }

    QString customExe = QFileDialog::getOpenFileName(
        this,
        tr("Select Executable to Run in Prefix"),
        startDir,
        tr("Windows Executables (*.exe);;All Files (*)")
    );

    if (customExe.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_processManager->runExeInPrefix(*game, customExe, &errorMessage)) {
        QMessageBox::warning(this, tr("Run in Prefix Error"), errorMessage);
    }
}

void MainWindow::onEditGame(const QString &gameId)
{
    auto it = std::find_if(m_games.begin(), m_games.end(), [&](const Game &g) {
        return g.id == gameId;
    });

    if (it == m_games.end()) {
        return;
    }

    GameConfigDialog dialog(*it, this);
    if (dialog.exec() == QDialog::Accepted) {
        *it = dialog.game();
        updateGameInGrid(*it);
        LibraryManager::saveGames(m_games);
        updateToolbarState();
    }
}

void MainWindow::onRemoveGame(const QString &gameId)
{
    auto it = std::find_if(m_games.begin(), m_games.end(), [&](const Game &g) {
        return g.id == gameId;
    });

    if (it == m_games.end()) {
        return;
    }

    if (m_processManager->isGameRunning(gameId)) {
        QMessageBox::warning(this, tr("Cannot Delete"), tr("Please stop the game before deleting it."));
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(tr("Delete Game"));
    msgBox.setText(tr("Are you sure you want to delete \"%1\" from the library?").arg(it->title));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    // Checkbox para remover o prefixo Wine (desmarcada por padrão)
    QCheckBox *prefixCheckBox = nullptr;
    QString prefixPath = it->winePrefix.trimmed();
    if (it->runner == GameRunner::Wine && !prefixPath.isEmpty() && QDir(prefixPath).exists()) {
        prefixCheckBox = new QCheckBox(tr("Delete Wine prefix folder as well (%1)").arg(prefixPath), &msgBox);
        prefixCheckBox->setChecked(false);
        msgBox.setCheckBox(prefixCheckBox);
    }

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    // Se marcado, remover a pasta do prefixo do disco
    if (prefixCheckBox && prefixCheckBox->isChecked() && !prefixPath.isEmpty()) {
        QDir prefixDir(prefixPath);
        if (prefixDir.exists()) {
            prefixDir.removeRecursively();
        }
    }

    for (int i = 0; i < m_gameListWidget->count(); ++i) {
        if (m_gameListWidget->item(i)->data(Qt::UserRole).toString() == gameId) {
            delete m_gameListWidget->takeItem(i);
            break;
        }
    }

    m_games.erase(it);
    LibraryManager::saveGames(m_games);
    updateToolbarState();
}

void MainWindow::onGameStarted(const QString &gameId)
{
    const Game *game = findGame(gameId);
    for (int i = 0; i < m_gameListWidget->count(); ++i) {
        QListWidgetItem *item = m_gameListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == gameId) {
            if (game) {
                item->setText(game->title);
                item->setIcon(IconHelper::createGameIcon(game->title, game->iconPath, 128, true));
            }
            break;
        }
    }
    updateToolbarState();
}

void MainWindow::onGameFinished(const QString &gameId, int /*exitCode*/)
{
    const Game *game = findGame(gameId);
    for (int i = 0; i < m_gameListWidget->count(); ++i) {
        QListWidgetItem *item = m_gameListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == gameId) {
            if (game) {
                item->setText(game->title);
                item->setIcon(IconHelper::createGameIcon(game->title, game->iconPath, 128, false));
            }
            break;
        }
    }
    updateToolbarState();
}

void MainWindow::onGameError(const QString &gameId, const QString &errorMessage)
{
    QMessageBox::critical(this, tr("Game Launch Error"), errorMessage);
    updateToolbarState();
}




