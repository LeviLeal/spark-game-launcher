#include "runnerdetector.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QSet>

QList<WineVersionInfo> RunnerDetector::detectWineVersions()
{
    QList<WineVersionInfo> versions;
    QSet<QString> seenPaths;

    auto addVersion = [&](const QString &name, const QString &path) {
        if (path.isEmpty()) {
            return;
        }
        QString canonical = QFileInfo(path).canonicalFilePath();
        if (canonical.isEmpty()) {
            canonical = path;
        }
        if (!seenPaths.contains(canonical)) {
            seenPaths.insert(canonical);
            versions.append({name, canonical});
        }
    };

    // 1. Search custom Steam compatibility tools
    QString home = QDir::homePath();
    QStringList customToolsDirs = {
        home + QStringLiteral("/.local/share/Steam/compatibilitytools.d"),
        home + QStringLiteral("/.steam/root/compatibilitytools.d"),
        home + QStringLiteral("/.steam/steam/compatibilitytools.d"),
        QStringLiteral("/usr/share/steam/compatibilitytools.d")
    };

    // 2. Search additional Steam libraries via libraryfolders.vdf
    QString vdfPath = home + QStringLiteral("/.local/share/Steam/steamapps/libraryfolders.vdf");
    if (!QFile::exists(vdfPath)) {
        vdfPath = home + QStringLiteral("/.steam/steam/steamapps/libraryfolders.vdf");
    }

    QStringList libraryFolders;
    libraryFolders.append(home + QStringLiteral("/.local/share/Steam"));

    if (QFile::exists(vdfPath)) {
        QFile file(vdfPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            QRegularExpression pathRegex(QStringLiteral(R"lit("path"\s+"([^"]+)")lit"));
            QRegularExpressionMatchIterator it = pathRegex.globalMatch(content);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString libPath = match.captured(1);
                if (!libraryFolders.contains(libPath)) {
                    libraryFolders.append(libPath);
                }
            }
        }
    }

    for (const QString &libFolder : libraryFolders) {
        QString toolsDir = libFolder + QStringLiteral("/compatibilitytools.d");
        if (!customToolsDirs.contains(toolsDir)) {
            customToolsDirs.append(toolsDir);
        }
    }

    // Scan compatibilitytools.d directories (e.g. GE-Proton)
    for (const QString &dirPath : customToolsDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }
        const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            QString protonBin = entry.filePath() + QStringLiteral("/proton");
            if (QFile::exists(protonBin)) {
                addVersion(entry.fileName(), protonBin);
            } else {
                addVersion(entry.fileName(), entry.filePath());
            }
        }
    }

    // Scan official Proton versions installed in steamapps/common
    for (const QString &libFolder : libraryFolders) {
        QDir commonDir(libFolder + QStringLiteral("/steamapps/common"));
        if (!commonDir.exists()) {
            continue;
        }
        const QFileInfoList entries = commonDir.entryInfoList(
            QStringList{QStringLiteral("Proton*")},
            QDir::Dirs | QDir::NoDotAndDotDot
        );
        for (const QFileInfo &entry : entries) {
            QString protonBin = entry.filePath() + QStringLiteral("/proton");
            if (QFile::exists(protonBin)) {
                addVersion(entry.fileName(), protonBin);
            } else {
                addVersion(entry.fileName(), entry.filePath());
            }
        }
    }

    // 3. Search system Wine
    QString wineBin = QStandardPaths::findExecutable(QStringLiteral("wine"));
    if (!wineBin.isEmpty()) {
        addVersion(QStringLiteral("System Wine (") + wineBin + QStringLiteral(")"), wineBin);
    }

    // Fallback option if nothing was found
    if (versions.isEmpty()) {
        versions.append({QStringLiteral("Default Wine (wine)"), QStringLiteral("wine")});
    }

    return versions;
}
