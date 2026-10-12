#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QMap>
#include <QProcess>
#include "../models/game.h"

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject *parent = nullptr);
    ~ProcessManager() override;

    bool isGameRunning(const QString &gameId) const;
    bool startGame(const Game &game, bool detached = false);
    bool stopGame(const QString &gameId);

    bool openWinetricks(const Game &game, QString *errorMessage = nullptr);
    bool runExeInPrefix(const Game &game, const QString &customExePath, QString *errorMessage = nullptr);

    QString getGameLog(const QString &gameId) const;
    void clearGameLog(const QString &gameId);

signals:
    void gameStarted(const QString &gameId);
    void gameFinished(const QString &gameId, int exitCode);
    void gameError(const QString &gameId, const QString &errorMessage);
    void logReceived(const QString &gameId, const QString &text);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess* createProcessForGame(const Game &game);
    void handleProcessOutput(QProcess *process, const QByteArray &data);

    QMap<QString, QProcess*> m_processes;
    QMap<QProcess*, QString> m_processToGameId;
    QMap<QString, QString> m_gameLogs;
};

#endif // PROCESSMANAGER_H
