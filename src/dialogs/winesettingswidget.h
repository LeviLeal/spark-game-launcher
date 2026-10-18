#ifndef WINESETTINGSWIDGET_H
#define WINESETTINGSWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;

class WineSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WineSettingsWidget(QWidget *parent = nullptr);
    ~WineSettingsWidget() override = default;

public slots:
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onWineVersionChanged(int index);
    void onBrowsePrefix();
    void onPrefixEdited(const QString &text);

private:
    void setupUi();
    void populateWineVersions();

    QComboBox *m_wineVersionCombo{nullptr};
    QLineEdit *m_prefixEdit{nullptr};
    QPushButton *m_browsePrefixBtn{nullptr};
};

#endif // WINESETTINGSWIDGET_H
