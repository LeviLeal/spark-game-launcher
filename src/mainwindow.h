#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "models/game.h"
#include <QList>

class QToolBar;
class QAction;
class QListWidget;
class QListWidgetItem;
class ProcessManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    QMenu *createPopupMenu() override;
    void changeEvent(QEvent *event) override;

private slots:
    void onAddGameClicked();
    void onEditGameClicked();
    void onRemoveGameClicked();
    void onPlayPauseClicked();
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onContextMenuRequested(const QPoint &pos);
    void onEditGame(const QString &gameId);
    void onRemoveGame(const QString &gameId);
    void onOpenWinetricks(const QString &gameId);
    void onRunExeInPrefix(const QString &gameId);
    void onViewLogs(const QString &gameId);
    void onSettingsClicked();

    void onGameStarted(const QString &gameId);
    void onGameFinished(const QString &gameId, int exitCode);
    void onGameError(const QString &gameId, const QString &errorMessage);

private:
    void setupToolBar();
    void setupCentralWidget();
    void loadLibrary();
    void addGameToGrid(const Game &game);
    void updateGameInGrid(const Game &game);
    void updateToolbarState();
    void updateToolBarIcons();
    void syncIconTheme();
    void applyThemeChange();
    void updateGameListStyle();
    void toggleGame(const QString &gameId);
    void launchGame(const QString &gameId, bool closeOnLaunch = false);
    QString selectedGameId() const;
    const Game* findGame(const QString &gameId) const;

    QToolBar *m_toolBar{nullptr};
    QAction *m_addGameAction{nullptr};
    QAction *m_editGameAction{nullptr};
    QAction *m_removeGameAction{nullptr};
    QAction *m_playPauseAction{nullptr};
    QAction *m_settingsAction{nullptr};

    QListWidget *m_gameListWidget{nullptr};
    QList<Game> m_games;

    ProcessManager *m_processManager{nullptr};
};

#endif // MAINWINDOW_H
