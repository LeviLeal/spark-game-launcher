#include "librarymanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QString LibraryManager::getLibraryFilePath()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + QStringLiteral("/.config/spark");
    }
    QDir().mkpath(configDir);
    return configDir + QStringLiteral("/games.json");
}

QList<Game> LibraryManager::loadGames()
{
    QList<Game> games;
    QString path = getLibraryFilePath();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return games;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return games;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        Game g;
        g.id = obj.value(QStringLiteral("id")).toString();
        g.title = obj.value(QStringLiteral("title")).toString();
        g.executablePath = obj.value(QStringLiteral("executablePath")).toString();
        g.runner = static_cast<GameRunner>(obj.value(QStringLiteral("runner")).toInt(0));
        g.winePrefix = obj.value(QStringLiteral("winePrefix")).toString();
        g.wineVersion = obj.value(QStringLiteral("wineVersion")).toString();
        g.iconPath = obj.value(QStringLiteral("iconPath")).toString();
        g.preCommand = obj.value(QStringLiteral("preCommand")).toString();

        if (!g.id.isEmpty() && !g.title.isEmpty()) {
            games.append(g);
        }
    }

    return games;
}

bool LibraryManager::saveGames(const QList<Game> &games)
{
    QString path = getLibraryFilePath();
    QJsonArray array;

    for (const Game &g : games) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), g.id);
        obj.insert(QStringLiteral("title"), g.title);
        obj.insert(QStringLiteral("executablePath"), g.executablePath);
        obj.insert(QStringLiteral("runner"), static_cast<int>(g.runner));
        obj.insert(QStringLiteral("winePrefix"), g.winePrefix);
        obj.insert(QStringLiteral("wineVersion"), g.wineVersion);
        obj.insert(QStringLiteral("iconPath"), g.iconPath);
        obj.insert(QStringLiteral("preCommand"), g.preCommand);
        array.append(obj);
    }

    QJsonDocument doc(array);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}
