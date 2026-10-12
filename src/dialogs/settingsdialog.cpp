#include "settingsdialog.h"
#include "generalsettingswidget.h"
#include "protondownloadwidget.h"
#include "exportsettingswidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QPalette>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    resize(940, 540);
    setMinimumSize(800, 460);

    setupUi();
}

void SettingsDialog::setupUi()
{
    // Layout principal com margens 0 para encaixar a barra lateral flush à esquerda (estilo Dolphin Emulator)
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Barra lateral inspirada no StackedSettingsWindow do Dolphin Emulator
    m_categoryList = new QListWidget(this);
    m_categoryList->setObjectName(QStringLiteral("settingsSidebar"));
    m_categoryList->setFrameShape(QFrame::NoFrame);
    m_categoryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_categoryList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_categoryList->setFixedWidth(195);
    m_categoryList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    updateSidebarStyle();

    mainLayout->addWidget(m_categoryList);

    // Lado direito: conteúdo empilhado e botão de fechar
    auto *rightSide = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setContentsMargins(18, 16, 18, 14);
    rightLayout->setSpacing(12);

    m_stackedWidget = new QStackedWidget(rightSide);
    rightLayout->addWidget(m_stackedWidget, 1);

    // Botão Fechar no canto inferior direito (como no Dolphin Emulator)
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, rightSide);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    rightLayout->addWidget(m_buttonBox);

    mainLayout->addWidget(rightSide, 1);

    // Conectar navegação da lista ao stacked widget
    connect(m_categoryList, &QListWidget::currentRowChanged,
            m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // Panes de configuração
    m_generalSettingsWidget = new GeneralSettingsWidget(m_stackedWidget);
    addPane(m_generalSettingsWidget, tr("Desktop Integration"));

    m_protonDownloadWidget = new ProtonDownloadWidget(m_stackedWidget);
    addPane(m_protonDownloadWidget, tr("Proton Downloads"));

    m_exportSettingsWidget = new ExportSettingsWidget(m_stackedWidget);
    connect(m_exportSettingsWidget, &ExportSettingsWidget::dataImported, this, [this]() {
        m_libraryModified = true;
        emit libraryChanged();
    });
    addPane(m_exportSettingsWidget, tr("Export / Import"));

    m_categoryList->setCurrentRow(0);
}

void SettingsDialog::addPane(QWidget *widget, const QString &name)
{
    m_stackedWidget->addWidget(widget);
    auto *item = new QListWidgetItem(name, m_categoryList);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void SettingsDialog::updateSidebarStyle()
{
    if (!m_categoryList) {
        return;
    }

    m_categoryList->setStyleSheet(QStringLiteral(
        "QListWidget#settingsSidebar {"
        "    background-color: palette(alternate-base);"
        "    border: none;"
        "    border-right: 1px solid palette(mid);"
        "    outline: none;"
        "}"
        "QListWidget#settingsSidebar::item {"
        "    padding: 8px 14px;"
        "    border: none;"
        "}"
        "QListWidget#settingsSidebar::item:hover {"
        "    background-color: rgba(128, 128, 128, 0.15);"
        "}"
        "QListWidget#settingsSidebar::item:selected {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "}"
    ));
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ThemeChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        updateSidebarStyle();
    }
    QDialog::changeEvent(event);
}
