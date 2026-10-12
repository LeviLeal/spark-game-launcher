#ifndef UMUMANAGER_H
#define UMUMANAGER_H

#include <QObject>
#include <QString>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QWidget;

class UmuManager : public QObject
{
    Q_OBJECT

public:
    explicit UmuManager(QObject *parent = nullptr);
    ~UmuManager() override;

    static QString sparkUmuPath();
    static QString findUmuBin();
    static bool isUmuAvailable();

    // Assegura que o UMU está instalado. Se não estiver, pergunta ao usuário e exibe progresso.
    // callback(true) se instalado ou baixado com sucesso, callback(false) se cancelado ou erro.
    static void ensureUmuInstalled(QWidget *parent, std::function<void(bool success)> callback);

    void startDownload(QWidget *parent = nullptr, std::function<void(bool success)> callback = nullptr);

signals:
    void downloadProgress(qint64 received, qint64 total);
    void finished(bool success, const QString &message = QString());

private:
    void fetchReleaseInfo();
    void downloadTar(const QString &url);
    void extractTar(const QString &tarFilePath);

    QNetworkAccessManager *m_nam{nullptr};
    QProgressDialog *m_dialog{nullptr};
    std::function<void(bool)> m_callback;
};

#endif // UMUMANAGER_H
