#include <QApplication>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Spark"));
    app.setOrganizationName(QStringLiteral("SparkLauncher"));

    const QString serverName = QStringLiteral("spark-game-launcher-") +
        (qEnvironmentVariableIsSet("USER") ? qEnvironmentVariable("USER") : QStringLiteral("default"));

    // Tenta conectar a uma instância já em execução
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        // Instância existente encontrada: sinaliza para focar a janela e encerra esta nova instância
        socket.write("RAISE\n");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    // Se não conectou, remove qualquer socket órfão remanescente de encerramento inesperado
    QLocalServer::removeServer(serverName);

    QLocalServer server;
    if (!server.listen(serverName)) {
        QLocalServer::removeServer(serverName);
        server.listen(serverName);
    }

    QIcon appIcon(QStringLiteral(":/logo.png"));
    if (appIcon.isNull()) {
        appIcon = QIcon(QStringLiteral("logo.png"));
    }
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    // Ao receber conexão de uma segunda instância, restaura e foca a janela atual
    QObject::connect(&server, &QLocalServer::newConnection, [&server, &window]() {
        while (QLocalSocket *client = server.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
            client->disconnectFromServer();
        }
        if (window.isMinimized()) {
            window.showNormal();
        }
        window.show();
        window.raise();
        window.activateWindow();
    });

    return app.exec();
}
