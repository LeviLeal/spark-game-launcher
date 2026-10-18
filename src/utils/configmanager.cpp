#include "configmanager.h"
#include "runnerdetector.h"

#include <QSettings>
#include <QDir>
#include <QRegularExpression>

QString ConfigManager::defaultWineVersion()
{
    QSettings settings;
    QString version = settings.value(QStringLiteral("wine/defaultVersion")).toString().trimmed();
    if (!version.isEmpty()) {
        return version;
    }

    // Fallback: seleciona o primeiro runner detectado no sistema
    const auto versions = RunnerDetector::detectWineVersions();
    if (!versions.isEmpty()) {
        return versions.first().path;
    }
    return QStringLiteral("wine");
}

void ConfigManager::setDefaultWineVersion(const QString &version)
{
    QSettings settings;
    settings.setValue(QStringLiteral("wine/defaultVersion"), version.trimmed());
}

QString ConfigManager::defaultWinePrefix()
{
    QSettings settings;
    QString prefix = settings.value(QStringLiteral("wine/defaultPrefix")).toString().trimmed();
    if (!prefix.isEmpty() && prefix != QDir::homePath() + QStringLiteral("/.local/share/spark/prefixes")) {
        return prefix;
    }
    return QDir::homePath() + QStringLiteral("/.local/share/spark/prefixes/default");
}

void ConfigManager::setDefaultWinePrefix(const QString &prefix)
{
    QSettings settings;
    QString clean = prefix.trimmed();
    if (clean.isEmpty()) {
        clean = QDir::homePath() + QStringLiteral("/.local/share/spark/prefixes/default");
    }
    settings.setValue(QStringLiteral("wine/defaultPrefix"), clean);
}

QString ConfigManager::defaultPrefixForGame(const QString &/*gameTitleOrId*/)
{
    return defaultWinePrefix();
}
