#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QIcon>
#include <QString>

class IconHelper {
public:
    static QPixmap generateLetterPixmap(const QString &title, int size = 128);
    static QIcon generateLetterIcon(const QString &title, int size = 128);
    static QIcon createGameIcon(const QString &title, const QString &iconPath = QString(), int size = 128, bool isRunning = false);
    static bool isIcoutilsAvailable();
    static QString extractExeIcon(const QString &exePath);
};

#endif // ICONHELPER_H
