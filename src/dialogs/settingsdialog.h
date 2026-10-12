#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class ProtonDownloadWidget;
class GeneralSettingsWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

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
    ProtonDownloadWidget *m_protonDownloadWidget{nullptr};
};

#endif // SETTINGSDIALOG_H
