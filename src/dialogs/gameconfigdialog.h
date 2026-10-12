#ifndef GAMECONFIGDIALOG_H
#define GAMECONFIGDIALOG_H

#include <QDialog>
#include "../models/game.h"

class QLineEdit;
class QComboBox;
class QPushButton;
class QGroupBox;
class QDialogButtonBox;
class QLabel;

class GameConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GameConfigDialog(QWidget *parent = nullptr);
    explicit GameConfigDialog(const Game &game, QWidget *parent = nullptr);
    ~GameConfigDialog() override = default;

    void setGame(const Game &game);
    Game game() const;

private slots:
    void onRunnerChanged(int index);
    void onBrowseExecutableClicked();
    void onBrowsePrefixClicked();
    void onBrowseIconClicked();
    void onAcceptClicked();

private:
    void setupUi();
    void populateWineVersions();
    void updateIconPreview(const QString &path);

    QString m_gameId;
    QLineEdit *m_titleEdit{nullptr};
    QLineEdit *m_executableEdit{nullptr};
    QPushButton *m_browseExecutableBtn{nullptr};
    QLineEdit *m_iconEdit{nullptr};
    QPushButton *m_browseIconBtn{nullptr};
    QLabel *m_iconPreview{nullptr};
    QComboBox *m_runnerCombo{nullptr};

    QGroupBox *m_nativeGroup{nullptr};
    QLineEdit *m_preCommandEdit{nullptr};

    QGroupBox *m_wineGroup{nullptr};
    QLineEdit *m_prefixEdit{nullptr};
    QPushButton *m_browsePrefixBtn{nullptr};
    QComboBox *m_wineVersionCombo{nullptr};

    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // GAMECONFIGDIALOG_H
