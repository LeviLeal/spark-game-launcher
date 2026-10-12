#ifndef EXPORTSETTINGSWIDGET_H
#define EXPORTSETTINGSWIDGET_H

#include <QWidget>

class QCheckBox;
class QPushButton;
class QLabel;
class QListWidget;
class QListWidgetItem;

class ExportSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExportSettingsWidget(QWidget *parent = nullptr);
    ~ExportSettingsWidget() override = default;

public slots:
    void refreshSizes();

signals:
    void dataImported();

private slots:
    void onExportClicked();
    void onImportClicked();
    void onPrefixesCheckToggled(bool checked);
    void onPrefixItemChanged(QListWidgetItem *item);
    void onSelectAllPrefixes();
    void onDeselectAllPrefixes();

private:
    void setupUi();
    void populatePrefixList();
    void updatePrefixSummary();
    static quint64 getDirSize(const QString &dirPath);
    static QString formatSize(quint64 bytes);

    QCheckBox *m_configCheck{nullptr};
    QCheckBox *m_iconsCheck{nullptr};
    QCheckBox *m_prefixesCheck{nullptr};
    QWidget *m_prefixContainer{nullptr};
    QListWidget *m_prefixListWidget{nullptr};
    QPushButton *m_selectAllPrefixesBtn{nullptr};
    QPushButton *m_deselectAllPrefixesBtn{nullptr};

    QCheckBox *m_toolsCheck{nullptr};

    QPushButton *m_exportBtn{nullptr};
    QPushButton *m_importBtn{nullptr};
    QPushButton *m_refreshBtn{nullptr};

    bool m_updatingPrefixSummary{false};
};

#endif // EXPORTSETTINGSWIDGET_H
