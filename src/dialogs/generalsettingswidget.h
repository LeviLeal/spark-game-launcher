#ifndef GENERALSETTINGSWIDGET_H
#define GENERALSETTINGSWIDGET_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class GeneralSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralSettingsWidget(QWidget *parent = nullptr);
    ~GeneralSettingsWidget() override = default;

public slots:
    void refreshStatus();

private slots:
    void onAddOrUpdateDesktop();
    void onRemoveDesktop();

private:
    void setupUi();
    QString currentExecutablePath() const;
    QString desktopFilePath() const;
    QString iconFilePath() const;
    QString readExecFromDesktop(const QString &filePath) const;
    void runDesktopDatabaseUpdate();

    QLineEdit *m_currentExeEdit{nullptr};
    QLineEdit *m_desktopPathEdit{nullptr};
    QPushButton *m_addOrUpdateBtn{nullptr};
    QPushButton *m_removeBtn{nullptr};
};

#endif // GENERALSETTINGSWIDGET_H
