#include "logviewerdialog.h"
#include "../utils/processmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFontDatabase>
#include <QClipboard>
#include <QGuiApplication>

LogViewerDialog::LogViewerDialog(const QString &gameId,
                                 const QString &gameTitle,
                                 ProcessManager *processManager,
                                 QWidget *parent)
    : QDialog(parent)
    , m_gameId(gameId)
    , m_gameTitle(gameTitle)
    , m_processManager(processManager)
{
    setWindowTitle(tr("Game Logs - %1").arg(m_gameTitle));
    resize(760, 480);
    setupUi();

    if (m_processManager) {
        // Carregar o último log capturado
        QString currentLog = m_processManager->getGameLog(m_gameId);
        if (currentLog.isEmpty()) {
            m_logEdit->setPlainText(tr("[Spark] No logs available for this game yet. Run the game to generate logs.\n"));
        } else {
            m_logEdit->setPlainText(currentLog);
        }
        m_logEdit->moveCursor(QTextCursor::End);

        // Conectar para atualizações em tempo real
        connect(m_processManager, &ProcessManager::logReceived,
                this, &LogViewerDialog::onLogReceived);
        connect(m_processManager, &ProcessManager::gameStarted,
                this, &LogViewerDialog::onGameStarted);
    }
}

void LogViewerDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Fonte monospace
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_logEdit->setFont(monoFont);

    // Estilo terminal limpo
    m_logEdit->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "    background-color: #1a1c23;"
        "    color: #e0e4ec;"
        "    border: 1px solid rgba(255, 255, 255, 0.1);"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "}"
    ));

    mainLayout->addWidget(m_logEdit);

    // Barra inferior de botões
    auto *buttonLayout = new QHBoxLayout();
    m_copyButton = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy to Clipboard"), this);
    connect(m_copyButton, &QPushButton::clicked, this, &LogViewerDialog::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    buttonLayout->addStretch();

    m_closeButton = new QPushButton(tr("Close"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);
}

void LogViewerDialog::onLogReceived(const QString &gameId, const QString &text)
{
    if (gameId == m_gameId) {
        m_logEdit->moveCursor(QTextCursor::End);
        m_logEdit->insertPlainText(text);
        m_logEdit->moveCursor(QTextCursor::End);
    }
}

void LogViewerDialog::onGameStarted(const QString &gameId)
{
    if (gameId == m_gameId) {
        // Ao executar de novo, limpa a tela de logs e volta a computar
        m_logEdit->clear();
    }
}

void LogViewerDialog::onCopyToClipboard()
{
    if (m_logEdit) {
        QGuiApplication::clipboard()->setText(m_logEdit->toPlainText());
    }
}
