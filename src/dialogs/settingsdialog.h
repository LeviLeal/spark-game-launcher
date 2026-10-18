#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class ProtonDownloadWidget;
class GeneralSettingsWidget;
class ExportSettingsWidget;
class WineSettingsWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

    bool isLibraryModified() const { return m_libraryModified; }

signals:
    void libraryChanged();

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupUi();
    void addPane(QWidget *widget, const QString &name);
    void updateSidebarStyle();

    QListWidget *m_categoryList{nullptr};
    QStackedWidget *m_stackedWidget{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    GeneralSettingsWidget *m_generalSettingsWidget{nullptr};
    WineSettingsWidget *m_wineSettingsWidget{nullptr};
    ProtonDownloadWidget *m_protonDownloadWidget{nullptr};
    ExportSettingsWidget *m_exportSettingsWidget{nullptr};

    bool m_libraryModified{false};
};

#endif // SETTINGSDIALOG_H
