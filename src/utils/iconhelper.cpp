#include "iconhelper.h"

#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QFont>
#include <QColor>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QImageReader>

#include <QPolygonF>
#include <QPointF>
#include <QPen>

QPixmap IconHelper::generateLetterPixmap(const QString &title, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    // Determinar a primeira letra válida
    QString letter = QStringLiteral("?");
    for (const QChar &ch : title) {
        if (ch.isLetterOrNumber()) {
            letter = QString(ch).toUpper();
            break;
        }
    }

    // Gerar uma cor de fundo harmônica baseada no hash do título
    quint32 hash = qHash(title);
    int hue = static_cast<int>(hash % 360);
    // Cores agradáveis no tema escuro/KDE: saturação moderada e luminosidade confortável
    QColor bgColor = QColor::fromHsv(hue, 140, 65);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Desenhar ladrilho quadrado (com leve arredondamento de 6px para elegância visual)
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(0, 0, size, size, 6, 6);

    // Desenhar a letra inicial centralizada e em negrito
    QFont font = painter.font();
    font.setPixelSize(size / 2);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(245, 245, 245));

    painter.drawText(pixmap.rect(), Qt::AlignCenter, letter);
    painter.end();

    return pixmap;
}

QIcon IconHelper::generateLetterIcon(const QString &title, int size)
{
    return QIcon(generateLetterPixmap(title, size));
}

QIcon IconHelper::createGameIcon(const QString &title, const QString &iconPath, int size, bool isRunning)
{
    QPixmap result(size, size);
    result.fill(Qt::transparent);

    bool hasCustomIcon = false;
    if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
        QPixmap customPix(iconPath);
        if (!customPix.isNull()) {
            // Escala mantendo proporção para cobrir o quadrado do ícone
            QPixmap scaled = customPix.scaled(
                size, size,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );

            // Recorta o centro para garantir formato quadrado perfeito
            int cropX = (scaled.width() - size) / 2;
            int cropY = (scaled.height() - size) / 2;
            QPixmap cropped = scaled.copy(cropX, cropY, size, size);

            // Aplica cantos suavemente arredondados (6px) combinando com o design visual do Spark
            QPainter painter(&result);
            painter.setRenderHint(QPainter::Antialiasing);

            QPainterPath path;
            path.addRoundedRect(0, 0, size, size, 6, 6);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, cropped);
            painter.end();

            hasCustomIcon = true;
        }
    }

    if (!hasCustomIcon) {
        result = generateLetterPixmap(title, size);
    }

    if (isRunning) {
        QPainter badgePainter(&result);
        badgePainter.setRenderHint(QPainter::Antialiasing);

        int badgeSize = qRound(size * 0.26);
        int margin = qRound(size * 0.05);
        int bx = size - badgeSize - margin;
        int by = margin;

        // Dark outline for contrast against any background
        badgePainter.setPen(QPen(QColor(15, 18, 24, 230), 2.0));
        badgePainter.setBrush(QColor(46, 204, 113)); // #2ecc71 emerald green
        badgePainter.drawEllipse(bx, by, badgeSize, badgeSize);

        // White play triangle in center of badge
        badgePainter.setPen(Qt::NoPen);
        badgePainter.setBrush(Qt::white);
        qreal cx = bx + badgeSize / 2.0;
        qreal cy = by + badgeSize / 2.0;
        qreal r = badgeSize * 0.28;
        QPolygonF triangle;
        triangle << QPointF(cx - r * 0.65, cy - r * 0.95)
                 << QPointF(cx + r * 1.05, cy)
                 << QPointF(cx - r * 0.65, cy + r * 0.95);
        badgePainter.drawPolygon(triangle);
        badgePainter.end();
    }

    return QIcon(result);
}

bool IconHelper::isIcoutilsAvailable()
{
    static bool checked = false;
    static bool available = false;
    if (!checked) {
        available = !QStandardPaths::findExecutable(QStringLiteral("wrestool")).isEmpty() &&
                    !QStandardPaths::findExecutable(QStringLiteral("icotool")).isEmpty();
        checked = true;
    }
    return available;
}

QString IconHelper::extractExeIcon(const QString &exePath)
{
    if (exePath.isEmpty() || !QFile::exists(exePath) || !isIcoutilsAvailable()) {
        return QString();
    }

    // Diretório permanente: ~/.local/share/spark/icons
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + QStringLiteral("/.local/share");
    }
    QString iconsDir = dataDir + QStringLiteral("/spark/icons");
    QDir().mkpath(iconsDir);

    QFileInfo exeInfo(exePath);
    QString baseName = exeInfo.completeBaseName();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("game");
    }
    QString destFileName = QStringLiteral("%1_%2.png")
                               .arg(baseName)
                               .arg(QString::number(qHash(exePath), 16));
    QString destFilePath = iconsDir + QLatin1Char('/') + destFileName;

    // Se já foi extraído previamente e existe, reutiliza
    if (QFile::exists(destFilePath)) {
        return destFilePath;
    }

    // Cria diretório temporário para extração com icoutils
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return QString();
    }

    // 1. Extrai recursos de ícone (.ico) usando wrestool -x -t14
    QProcess wrestoolProc;
    wrestoolProc.start(QStringLiteral("wrestool"), {
        QStringLiteral("-x"),
        QStringLiteral("-t14"),
        QStringLiteral("-o"),
        tempDir.path(),
        exePath
    });
    if (!wrestoolProc.waitForFinished(5000) || wrestoolProc.exitCode() != 0) {
        return QString();
    }

    QDir tDir(tempDir.path());
    QStringList icoFiles = tDir.entryList({QStringLiteral("*.ico")}, QDir::Files);
    if (icoFiles.isEmpty()) {
        return QString();
    }

    // 2. Converte arquivos .ico para .png usando icotool -x
    for (const QString &icoFile : icoFiles) {
        QProcess icotoolProc;
        icotoolProc.start(QStringLiteral("icotool"), {
            QStringLiteral("-x"),
            QStringLiteral("-o"),
            tempDir.path(),
            tDir.filePath(icoFile)
        });
        icotoolProc.waitForFinished(5000);
    }

    // 3. Procura as imagens .png extraídas e escolhe a de maior resolução
    QStringList pngFiles = tDir.entryList({QStringLiteral("*.png")}, QDir::Files);
    if (pngFiles.isEmpty()) {
        return QString();
    }

    QString bestPngPath;
    int maxDimension = 0;

    for (const QString &pngFile : pngFiles) {
        QString fullPath = tDir.filePath(pngFile);
        QImageReader reader(fullPath);
        QSize sz = reader.size();
        int dimension = qMax(sz.width(), sz.height());
        if (dimension > maxDimension) {
            maxDimension = dimension;
            bestPngPath = fullPath;
        }
    }

    if (bestPngPath.isEmpty()) {
        bestPngPath = tDir.filePath(pngFiles.first());
    }

    // 4. Copia a melhor imagem PNG para o diretório permanente em ~/.local/share/spark/icons
    QFile::remove(destFilePath);
    if (QFile::copy(bestPngPath, destFilePath)) {
        return destFilePath;
    }

    return QString();
}
