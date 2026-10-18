#include "winesettingswidget.h"
#include "../utils/configmanager.h"
#include "../utils/runnerdetector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>

WineSettingsWidget::WineSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void WineSettingsWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(20);

    // Título simples sem descrição
    auto *titleLabel = new QLabel(tr("<b>Wine</b>"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() > 0 ? titleFont.pointSize() + 2 : 12);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Formulário com os dois campos diretos
    auto *formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setSpacing(14);

    // 1. Default runner
    m_wineVersionCombo = new QComboBox(this);
    m_wineVersionCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    formLayout->addRow(tr("Default runner:"), m_wineVersionCombo);

    // 2. Default wine prefix
    auto *prefixLayout = new QHBoxLayout();
    prefixLayout->setSpacing(6);
    prefixLayout->setContentsMargins(0, 0, 0, 0);

    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setPlaceholderText(tr("e.g. ~/.local/share/spark/prefixes/default"));

    m_browsePrefixBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString(), this);
    m_browsePrefixBtn->setToolTip(tr("Browse prefix directory..."));
    m_browsePrefixBtn->setFixedSize(32, 32);
    m_browsePrefixBtn->setIconSize(QSize(18, 18));

    prefixLayout->addWidget(m_prefixEdit, 1);
    prefixLayout->addWidget(m_browsePrefixBtn);

    formLayout->addRow(tr("Default wine prefix:"), prefixLayout);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch(1);

    // Conexões
    connect(m_wineVersionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WineSettingsWidget::onWineVersionChanged);
    connect(m_browsePrefixBtn, &QPushButton::clicked,
            this, &WineSettingsWidget::onBrowsePrefix);
    connect(m_prefixEdit, &QLineEdit::textEdited,
            this, &WineSettingsWidget::onPrefixEdited);
}

void WineSettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refresh();
}

void WineSettingsWidget::refresh()
{
    populateWineVersions();
    m_prefixEdit->setText(ConfigManager::defaultWinePrefix());
}

void WineSettingsWidget::populateWineVersions()
{
    QSignalBlocker blocker(m_wineVersionCombo);
    m_wineVersionCombo->clear();

    const auto versions = RunnerDetector::detectWineVersions();
    QString savedVersion = ConfigManager::defaultWineVersion();

    int selectedIdx = -1;
    for (int i = 0; i < versions.size(); ++i) {
        const auto &ver = versions.at(i);
        m_wineVersionCombo->addItem(ver.name, ver.path);
        if (ver.path == savedVersion || ver.name == savedVersion) {
            selectedIdx = i;
        }
    }

    if (selectedIdx < 0 && !savedVersion.isEmpty()) {
        m_wineVersionCombo->addItem(savedVersion, savedVersion);
        selectedIdx = m_wineVersionCombo->count() - 1;
    }

    if (selectedIdx >= 0) {
        m_wineVersionCombo->setCurrentIndex(selectedIdx);
    } else if (m_wineVersionCombo->count() > 0) {
        m_wineVersionCombo->setCurrentIndex(0);
        ConfigManager::setDefaultWineVersion(m_wineVersionCombo->currentData().toString());
    }
}

void WineSettingsWidget::onWineVersionChanged(int index)
{
    if (index < 0) {
        return;
    }
    QString path = m_wineVersionCombo->itemData(index).toString();
    if (path.isEmpty()) {
        path = m_wineVersionCombo->itemText(index);
    }
    ConfigManager::setDefaultWineVersion(path);
}

void WineSettingsWidget::onBrowsePrefix()
{
    QString current = m_prefixEdit->text().trimmed();
    if (current.isEmpty()) {
        current = ConfigManager::defaultWinePrefix();
    }

    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Default Wine Prefix Directory"),
        current
    );

    if (dir.isEmpty()) {
        return;
    }

    m_prefixEdit->setText(dir);
    ConfigManager::setDefaultWinePrefix(dir);
}

void WineSettingsWidget::onPrefixEdited(const QString &text)
{
    ConfigManager::setDefaultWinePrefix(text.trimmed());
}
