#ifndef RUNNERDETECTOR_H
#define RUNNERDETECTOR_H

#include <QString>
#include <QList>

struct WineVersionInfo {
    QString name;
    QString path;
};

class RunnerDetector {
public:
    static QList<WineVersionInfo> detectWineVersions();
};

#endif // RUNNERDETECTOR_H
