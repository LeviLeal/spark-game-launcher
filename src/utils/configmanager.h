#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>

class ConfigManager
{
public:
    static QString defaultWineVersion();
    static void setDefaultWineVersion(const QString &version);

    static QString defaultWinePrefix();
    static void setDefaultWinePrefix(const QString &prefix);

    static QString defaultPrefixForGame(const QString &gameTitleOrId);
};

#endif // CONFIGMANAGER_H
