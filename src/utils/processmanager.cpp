#include "processmanager.h"
#include "umumanager.h"
#include "configmanager.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QStandardPaths>
#include <QCoreApplication>

#include <unistd.h>
#include <signal.h>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
{
}

ProcessManager::~ProcessManager()
{
    // Ao fechar o launcher, os jogos devem continuar em execução no sistema.
    // Desconectamos os sinais para que não tentem emitir eventos e garantimos que
    // nenhum processo seja finalizado.
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        QProcess *proc = it.value();
        if (proc) {
            proc->disconnect();
            proc->setParent(nullptr);
        }
    }
    m_processes.clear();
    m_processToGameId.clear();
    m_gamePrefixes.clear();
}

bool ProcessManager::isGameRunning(const QString &gameId) const
{
    QProcess *proc = m_processes.value(gameId, nullptr);
    return proc && proc->state() != QProcess::NotRunning;
}

QProcess* ProcessManager::createProcessForGame(const Game &game)
{
    // Não associar 'this' como parent do QProcess para evitar que a destruição
    // do ProcessManager finalize o processo no SO através do ~QProcess()
    auto *process = new QProcess(nullptr);

    // Isolar a sessão e o grupo de processos para permitir que o jogo continue independente
    // e ignorar SIGPIPE para que a escrita de saída não finalize o processo caso os pipes fechem
    process->setChildProcessModifier([]() {
        ::setsid();
        ::signal(SIGPIPE, SIG_IGN);
    });

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString program;
    QStringList arguments;
    QString workingDir = QFileInfo(game.executablePath).dir().absolutePath();

    if (game.runner == GameRunner::Native) {
        QString preCmd = game.preCommand.trimmed();
        if (!preCmd.isEmpty()) {
            QStringList parts = QProcess::splitCommand(preCmd);
            if (!parts.isEmpty()) {
                program = parts.takeFirst();
                arguments = parts;
                arguments << game.executablePath;
            } else {
                program = game.executablePath;
            }
        } else {
            // Garantir permissão de execução caso seja executável direto
            QFile::setPermissions(game.executablePath,
                                  QFile::permissions(game.executablePath) | QFileDevice::ExeUser);
            program = game.executablePath;
        }
    } else {
        // Runner Wine / Proton
        QString wineVersion = game.wineVersion.trimmed();
        if (wineVersion.isEmpty()) {
            wineVersion = ConfigManager::defaultWineVersion();
        }
        bool isProton = wineVersion.contains(QStringLiteral("proton"), Qt::CaseInsensitive);

        // Se for um diretório contendo 'proton'
        QString protonBin = wineVersion;
        if (QFileInfo(wineVersion).isDir()) {
            QString potentialBin = wineVersion + QStringLiteral("/proton");
            if (QFile::exists(potentialBin)) {
                protonBin = potentialBin;
                isProton = true;
            }
        }

        QString prefix = game.winePrefix.trimmed();
        if (prefix.isEmpty()) {
            prefix = ConfigManager::defaultPrefixForGame(game.id);
        }
        QDir().mkpath(prefix);

        if (isProton) {
            QString protonDir = QFileInfo(protonBin).isDir() ? protonBin : QFileInfo(protonBin).dir().absolutePath();
            bool hasActivePrefix = isPrefixActive(prefix, protonDir);

            QString umuBin = UmuManager::findUmuBin();

            if (!umuBin.isEmpty()) {
                program = umuBin;
                arguments << game.executablePath;
                env.insert(QStringLiteral("PROTONPATH"), protonDir);
                env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
                env.insert(QStringLiteral("WINEPREFIX"), prefix);
                env.insert(QStringLiteral("GAMEID"), QStringLiteral("0"));

                if (hasActivePrefix) {
                    env.insert(QStringLiteral("PROTON_VERB"), QStringLiteral("runinprefix"));
                }
            } else {
                program = protonBin;
                QString verb = hasActivePrefix ? QStringLiteral("runinprefix") : QStringLiteral("run");
                arguments << verb << game.executablePath;

                QString steamClientPath = QDir::homePath() + QStringLiteral("/.steam/steam");
                if (!QDir(steamClientPath).exists()) {
                    steamClientPath = QDir::homePath() + QStringLiteral("/.local/share/Steam");
                }
                if (!QDir(steamClientPath).exists()) {
                    steamClientPath = QDir::homePath() + QStringLiteral("/.steam/root");
                }
                env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), steamClientPath);
                env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            }
        } else {
            // Wine tradicional
            program = wineVersion.isEmpty() ? QStringLiteral("wine") : wineVersion;
            arguments << game.executablePath;
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
        }

        // Suporte a pré-comando (como gamemoderun ou mangohud) para Wine / Proton
        QString preCmd = game.preCommand.trimmed();
        if (!preCmd.isEmpty()) {
            QStringList parts = QProcess::splitCommand(preCmd);
            if (!parts.isEmpty()) {
                arguments.prepend(program);
                for (int i = parts.size() - 1; i >= 1; --i) {
                    arguments.prepend(parts.at(i));
                }
                program = parts.first();
            }
        }
    }

    process->setProcessEnvironment(env);
    process->setProgram(program);
    process->setArguments(arguments);
    process->setWorkingDirectory(workingDir);

    return process;
}

bool ProcessManager::startGame(const Game &game, bool detached)
{
    if (isGameRunning(game.id)) {
        return false;
    }

    if (!QFile::exists(game.executablePath)) {
        emit gameError(game.id, tr("Executable not found: %1").arg(game.executablePath));
        return false;
    }

    // Ao executar de novo, limpa o log desse jogo e reinicia a contagem
    m_gameLogs[game.id].clear();

    QString time = QTime::currentTime().toString(QStringLiteral("hh:mm:ss"));
    QString startHeader = QStringLiteral("[%1] [Spark] Launching %2\n[%1] [Spark] Executable: %3\n")
        .arg(time, game.title, game.executablePath);
    if (game.runner == GameRunner::Wine) {
        startHeader += QStringLiteral("[%1] [Spark] Runner: Wine/Proton\n[%1] [Spark] Prefix: %2\n[%1] [Spark] Wine version: %3\n")
            .arg(time, game.winePrefix, game.wineVersion);
    }
    m_gameLogs[game.id].append(startHeader);
    emit logReceived(game.id, startHeader);

    QProcess *process = createProcessForGame(game);

    if (detached) {
        qint64 pid = 0;
        bool ok = process->startDetached(&pid);
        delete process;
        return ok;
    }

    m_processes.insert(game.id, process);
    m_processToGameId.insert(process, game.id);
    if (game.runner == GameRunner::Wine) {
        QString prefix = game.winePrefix.trimmed();
        if (prefix.isEmpty()) {
            prefix = ConfigManager::defaultWinePrefix();
        }
        m_gamePrefixes.insert(game.id, prefix);
    }

    QString gameId = game.id;
    connect(process, &QProcess::started, this, [this, gameId]() {
        emit gameStarted(gameId);
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        handleProcessOutput(process, process->readAllStandardOutput());
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        handleProcessOutput(process, process->readAllStandardError());
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &ProcessManager::onProcessError);

    process->start();
    return true;
}

bool ProcessManager::openWinetricks(const Game &game, QString *errorMessage)
{
    QString winetricksBin = QStandardPaths::findExecutable(QStringLiteral("winetricks"));
    if (winetricksBin.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("Winetricks is not installed on your system. Please install it using your package manager.");
        }
        return false;
    }

    QString prefix = game.winePrefix.trimmed();
    if (prefix.isEmpty()) {
        prefix = QDir::homePath() + QStringLiteral("/.local/share/spark/prefixes/") + game.id;
    }
    QDir().mkpath(prefix);

    QString wineVersion = game.wineVersion.trimmed();
    bool isProton = wineVersion.contains(QStringLiteral("proton"), Qt::CaseInsensitive);

    // No Proton, o prefixo de registro do Wine fica em <prefixo>/pfx
    QString actualPrefix = prefix;
    if (isProton || QDir(prefix + QStringLiteral("/pfx")).exists()) {
        actualPrefix = prefix + QStringLiteral("/pfx");
        QDir().mkpath(actualPrefix);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WINEPREFIX"), actualPrefix);

    if (isProton) {
        QString protonBin = wineVersion;
        if (QFileInfo(protonBin).isDir()) {
            protonBin += QStringLiteral("/proton");
        }
        QString protonDir = QFileInfo(protonBin).dir().absolutePath();
        QString internalWine = protonDir + QStringLiteral("/files/bin/wine");
        QString internalServer = protonDir + QStringLiteral("/files/bin/wineserver");
        if (QFile::exists(internalWine)) {
            env.insert(QStringLiteral("WINE"), internalWine);
        }
        if (QFile::exists(internalServer)) {
            env.insert(QStringLiteral("WINESERVER"), internalServer);
        }
    } else if (!wineVersion.isEmpty()) {
        env.insert(QStringLiteral("WINE"), wineVersion);
    }

    auto *proc = new QProcess(this);
    proc->setProcessEnvironment(env);
    proc->setProgram(winetricksBin);
    proc->setArguments(QStringList{QStringLiteral("--gui")});
    proc->setWorkingDirectory(prefix);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QObject::deleteLater);

    proc->start();
    return true;
}

bool ProcessManager::runExeInPrefix(const Game &game, const QString &customExePath, QString *errorMessage)
{
    if (!QFile::exists(customExePath)) {
        if (errorMessage) {
            *errorMessage = tr("Executable not found: %1").arg(customExePath);
        }
        return false;
    }

    Game customGame = game;
    customGame.executablePath = customExePath;

    QProcess *proc = createProcessForGame(customGame);
    if (!proc) {
        if (errorMessage) {
            *errorMessage = tr("Failed to initialize process for prefix.");
        }
        return false;
    }

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QObject::deleteLater);

    proc->start();
    return true;
}

bool ProcessManager::stopGame(const QString &gameId)
{
    QProcess *process = m_processes.value(gameId, nullptr);
    if (!process || process->state() == QProcess::NotRunning) {
        return false;
    }

    qint64 pid = process->processId();
    if (pid > 0) {
        ::kill(-pid, SIGTERM);
    }
    process->terminate();

    // Enviar SIGKILL caso não encerre em 2.5 segundos
    QTimer::singleShot(2500, this, [process, pid]() {
        if (process && process->state() != QProcess::NotRunning) {
            if (pid > 0) {
                ::kill(-pid, SIGKILL);
            }
            process->kill();
        }
    });

    return true;
}

void ProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
    auto *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }

    QString gameId = m_processToGameId.take(process);
    if (!gameId.isEmpty()) {
        m_processes.remove(gameId);
        m_gamePrefixes.remove(gameId);

        QString time = QTime::currentTime().toString(QStringLiteral("hh:mm:ss"));
        QString exitMsg = QStringLiteral("[%1] [Spark] Process finished with exit code %2.\n")
            .arg(time, QString::number(exitCode));
        m_gameLogs[gameId].append(exitMsg);
        emit logReceived(gameId, exitMsg);

        emit gameFinished(gameId, exitCode);
    }

    process->deleteLater();
}

void ProcessManager::onProcessError(QProcess::ProcessError error)
{
    auto *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }

    QString gameId = m_processToGameId.value(process);
    if (!gameId.isEmpty() && error != QProcess::Crashed) {
        QString time = QTime::currentTime().toString(QStringLiteral("hh:mm:ss"));
        QString errorMsg = QStringLiteral("[%1] [Spark] Error: %2\n")
            .arg(time, process->errorString());
        m_gameLogs[gameId].append(errorMsg);
        emit logReceived(gameId, errorMsg);

        emit gameError(gameId, process->errorString());
    }

    if (error == QProcess::FailedToStart) {
        m_processToGameId.remove(process);
        m_processes.remove(gameId);
        m_gamePrefixes.remove(gameId);
        process->deleteLater();
    }
}

void ProcessManager::handleProcessOutput(QProcess *process, const QByteArray &data)
{
    QString gameId = m_processToGameId.value(process);
    if (gameId.isEmpty() || data.isEmpty()) {
        return;
    }

    QString text = QString::fromUtf8(data);
    QString currentTime = QTime::currentTime().toString(QStringLiteral("hh:mm:ss"));
    QStringList lines = text.split(QLatin1Char('\n'));
    QString formatted;

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        if (line.isEmpty() && i == lines.size() - 1) {
            continue;
        }
        formatted += QStringLiteral("[%1] %2\n").arg(currentTime, line);
    }

    m_gameLogs[gameId].append(formatted);
    emit logReceived(gameId, formatted);
}

QString ProcessManager::getGameLog(const QString &gameId) const
{
    return m_gameLogs.value(gameId);
}

void ProcessManager::clearGameLog(const QString &gameId)
{
    m_gameLogs[gameId].clear();
}

bool ProcessManager::isPrefixActive(const QString &prefix, const QString &protonDir) const
{
    if (prefix.trimmed().isEmpty()) {
        return false;
    }

    QString canonicalTarget = QFileInfo(prefix).canonicalFilePath();
    if (canonicalTarget.isEmpty()) {
        canonicalTarget = prefix.trimmed();
    }

    // 1. Verifica se já existe um jogo executando no Spark com este mesmo prefixo
    for (auto it = m_gamePrefixes.constBegin(); it != m_gamePrefixes.constEnd(); ++it) {
        QString runningCanonical = QFileInfo(it.value()).canonicalFilePath();
        if (runningCanonical.isEmpty()) {
            runningCanonical = it.value().trimmed();
        }
        if (runningCanonical == canonicalTarget) {
            QProcess *proc = m_processes.value(it.key(), nullptr);
            if (proc && proc->state() != QProcess::NotRunning) {
                return true;
            }
        }
    }

    // 2. Verifica se o wineserver já está ativo para este prefixo no sistema
    QString wineserverBin = protonDir + QStringLiteral("/files/bin/wineserver");
    if (!QFile::exists(wineserverBin)) {
        wineserverBin = protonDir + QStringLiteral("/bin/wineserver");
    }
    if (!QFile::exists(wineserverBin)) {
        wineserverBin = QStandardPaths::findExecutable(QStringLiteral("wineserver"));
    }

    if (!wineserverBin.isEmpty()) {
        QProcess checkProc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
        checkProc.setProcessEnvironment(env);
        checkProc.start(wineserverBin, QStringList{QStringLiteral("-k0")});
        if (checkProc.waitForFinished(100)) {
            if (checkProc.exitCode() == 0) {
                return true;
            }
        }
    }

    return false;
}
