#ifndef GAME_H
#define GAME_H

#include <QString>

enum class GameRunner {
    Native,
    Wine
};

struct Game {
    QString id;
    QString title;
    QString executablePath;
    GameRunner runner{GameRunner::Native};
    QString winePrefix;
    QString wineVersion;
    QString iconPath;
    QString preCommand;
};

#endif // GAME_H
