#ifndef LOGVIEWERDIALOG_H
#define LOGVIEWERDIALOG_H

#include <QDialog>
#include <QString>

class QPlainTextEdit;
class QPushButton;
class ProcessManager;

class LogViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewerDialog(const QString &gameId,
                             const QString &gameTitle,
                             ProcessManager *processManager,
                             QWidget *parent = nullptr);
    ~LogViewerDialog() override = default;

private slots:
    void onLogReceived(const QString &gameId, const QString &text);
    void onGameStarted(const QString &gameId);
    void onCopyToClipboard();

private:
    void setupUi();

    QString m_gameId;
    QString m_gameTitle;
    ProcessManager *m_processManager{nullptr};

    QPlainTextEdit *m_logEdit{nullptr};
    QPushButton *m_copyButton{nullptr};
    QPushButton *m_closeButton{nullptr};
};

#endif // LOGVIEWERDIALOG_H
