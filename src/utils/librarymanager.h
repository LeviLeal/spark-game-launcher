#ifndef LIBRARYMANAGER_H
#define LIBRARYMANAGER_H

#include <QList>
#include "../models/game.h"

class LibraryManager {
public:
    static QList<Game> loadGames();
    static bool saveGames(const QList<Game> &games);

private:
    static QString getLibraryFilePath();
};

#endif // LIBRARYMANAGER_H
