#include "gameconfigdialog.h"
#include "../utils/runnerdetector.h"
#include "../utils/iconhelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>
#include <QUuid>

GameConfigDialog::GameConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Game Configuration"));
    setMinimumWidth(620);
    resize(640, 360);
    setupUi();
}

GameConfigDialog::GameConfigDialog(const Game &game, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Game"));
    setMinimumWidth(620);
    resize(640, 420);
    setupUi();
    setGame(game);
}

void GameConfigDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    auto *formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 1. Game Title
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Game title"));
    formLayout->addRow(tr("Game Title:"), m_titleEdit);

    // 2. Executable Path
    auto *exeLayout = new QHBoxLayout();
    m_executableEdit = new QLineEdit(this);
    m_executableEdit->setPlaceholderText(tr("Full path to executable"));
    m_browseExecutableBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), QString(), this);
    m_browseExecutableBtn->setToolTip(tr("Browse executable..."));
    m_browseExecutableBtn->setFixedSize(32, 32);
    m_browseExecutableBtn->setIconSize(QSize(18, 18));
    exeLayout->addWidget(m_executableEdit);
    exeLayout->addWidget(m_browseExecutableBtn);
    formLayout->addRow(tr("Executable:"), exeLayout);

    // 3. Custom Icon (optional)
    auto *iconLayout = new QHBoxLayout();
    m_iconEdit = new QLineEdit(this);
    m_iconEdit->setPlaceholderText(tr("Optional path to custom icon (*.png, *.jpg, *.ico, *.svg)"));
    m_browseIconBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("image-x-generic")), QString(), this);
    m_browseIconBtn->setToolTip(tr("Browse custom icon..."));
    m_browseIconBtn->setFixedSize(32, 32);
    m_browseIconBtn->setIconSize(QSize(18, 18));
    m_iconPreview = new QLabel(this);
    m_iconPreview->setFixedSize(32, 32);
    m_iconPreview->setAlignment(Qt::AlignCenter);
    m_iconPreview->setStyleSheet(QStringLiteral(
        "QLabel {"
        "    border: 1px solid palette(mid);"
        "    border-radius: 4px;"
        "    background-color: palette(base);"
        "}"
    ));
    iconLayout->addWidget(m_iconEdit);
    iconLayout->addWidget(m_iconPreview);
    iconLayout->addWidget(m_browseIconBtn);
    formLayout->addRow(tr("Icon:"), iconLayout);

    // 4. Runner
    m_runnerCombo = new QComboBox(this);
    m_runnerCombo->addItem(tr("Native"), static_cast<int>(GameRunner::Native));
    m_runnerCombo->addItem(tr("Wine"), static_cast<int>(GameRunner::Wine));
    formLayout->addRow(tr("Runner:"), m_runnerCombo);

    mainLayout->addLayout(formLayout);

    // 5. Native / Emulator options
    m_nativeGroup = new QGroupBox(tr("Native Settings"), this);
    m_nativeGroup->setFlat(true);
    auto *nativeFormLayout = new QFormLayout(m_nativeGroup);
    nativeFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    nativeFormLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_preCommandEdit = new QLineEdit(m_nativeGroup);
    m_preCommandEdit->setPlaceholderText(tr("Run a command before the executable"));
    nativeFormLayout->addRow(tr("Pre-command:"), m_preCommandEdit);

    mainLayout->addWidget(m_nativeGroup);

    // 6. Wine / Proton options
    m_wineGroup = new QGroupBox(tr("Wine settings"), this);
    m_wineGroup->setFlat(true);
    auto *wineFormLayout = new QFormLayout(m_wineGroup);
    wineFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    wineFormLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Prefix
    auto *prefixLayout = new QHBoxLayout();
    m_prefixEdit = new QLineEdit(m_wineGroup);
    m_prefixEdit->setPlaceholderText(tr("e.g. ~/.local/share/spark/prefixes/my_game"));
    m_browsePrefixBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), QString(), m_wineGroup);
    m_browsePrefixBtn->setToolTip(tr("Browse prefix folder..."));
    m_browsePrefixBtn->setFixedSize(32, 32);
    m_browsePrefixBtn->setIconSize(QSize(18, 18));
    prefixLayout->addWidget(m_prefixEdit);
    prefixLayout->addWidget(m_browsePrefixBtn);
    wineFormLayout->addRow(tr("Prefix:"), prefixLayout);

    // Wine / Proton version
    m_wineVersionCombo = new QComboBox(m_wineGroup);
    populateWineVersions();
    wineFormLayout->addRow(tr("Wine version:"), m_wineVersionCombo);

    mainLayout->addWidget(m_wineGroup);

    // Initially Native is default
    m_nativeGroup->setVisible(true);
    m_wineGroup->setVisible(false);

    // Save / Cancel buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    mainLayout->addWidget(m_buttonBox);

    // Connections
    connect(m_runnerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GameConfigDialog::onRunnerChanged);
    connect(m_browseExecutableBtn, &QPushButton::clicked,
            this, &GameConfigDialog::onBrowseExecutableClicked);
    connect(m_browseIconBtn, &QPushButton::clicked,
            this, &GameConfigDialog::onBrowseIconClicked);
    connect(m_iconEdit, &QLineEdit::textChanged,
            this, &GameConfigDialog::updateIconPreview);
    connect(m_browsePrefixBtn, &QPushButton::clicked,
            this, &GameConfigDialog::onBrowsePrefixClicked);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &GameConfigDialog::onAcceptClicked);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void GameConfigDialog::populateWineVersions()
{
    m_wineVersionCombo->clear();
    const auto versions = RunnerDetector::detectWineVersions();
    for (const auto &ver : versions) {
        m_wineVersionCombo->addItem(ver.name, ver.path);
    }
}

void GameConfigDialog::onRunnerChanged(int index)
{
    auto runner = static_cast<GameRunner>(m_runnerCombo->itemData(index).toInt());
    bool isWine = (runner == GameRunner::Wine);
    m_wineGroup->setVisible(isWine);
    m_nativeGroup->setVisible(!isWine);
    adjustSize();
}

void GameConfigDialog::onBrowseExecutableClicked()
{
    QString current = m_executableEdit->text().trimmed();
    QString startDir = current.isEmpty() ? QDir::homePath() : QFileInfo(current).absolutePath();

    QString file = QFileDialog::getOpenFileName(
        this,
        tr("Select Executable or Game File"),
        startDir,
        tr("All Supported Files (*.exe *.sh *.bin *.x86_64 *.iso *.cue *.chd *.rvz *.rom *.wbfs);;Executables (*.exe *.sh *.bin *.x86_64);;Disc Images / ROMs (*.iso *.cue *.chd *.rvz *.rom *.wbfs);;All Files (*)")
    );

    if (file.isEmpty()) {
        return;
    }

    m_executableEdit->setText(file);

    // Suggest title if empty
    if (m_titleEdit->text().trimmed().isEmpty()) {
        m_titleEdit->setText(QFileInfo(file).completeBaseName());
    }

    // Auto-select Wine if executable is .exe
    if (file.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        int wineIndex = m_runnerCombo->findData(static_cast<int>(GameRunner::Wine));
        if (wineIndex >= 0) {
            m_runnerCombo->setCurrentIndex(wineIndex);
        }

        // Auto-extract icon if icoutils is available and icon field is empty
        if (m_iconEdit->text().trimmed().isEmpty() && IconHelper::isIcoutilsAvailable()) {
            QString extractedIcon = IconHelper::extractExeIcon(file);
            if (!extractedIcon.isEmpty()) {
                m_iconEdit->setText(extractedIcon);
                updateIconPreview(extractedIcon);
            }
        }
    }
}

void GameConfigDialog::onBrowsePrefixClicked()
{
    QString current = m_prefixEdit->text().trimmed();
    QString startDir = current.isEmpty() ? QDir::homePath() : current;

    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Wine Prefix Directory"),
        startDir
    );

    if (!dir.isEmpty()) {
        m_prefixEdit->setText(dir);
    }
}

void GameConfigDialog::onBrowseIconClicked()
{
    QString current = m_iconEdit->text().trimmed();
    QString startDir = current.isEmpty() ? QDir::homePath() : QFileInfo(current).absolutePath();

    QString file = QFileDialog::getOpenFileName(
        this,
        tr("Select Game Icon"),
        startDir,
        tr("Images (*.png *.jpg *.jpeg *.svg *.ico *.webp);;All Files (*)")
    );

    if (!file.isEmpty()) {
        m_iconEdit->setText(file);
    }
}

void GameConfigDialog::updateIconPreview(const QString &path)
{
    QString trimmed = path.trimmed();
    if (!trimmed.isEmpty() && QFile::exists(trimmed)) {
        QPixmap pix(trimmed);
        if (!pix.isNull()) {
            m_iconPreview->setPixmap(pix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_iconPreview->setToolTip(trimmed);
            return;
        }
    }
    m_iconPreview->clear();
    m_iconPreview->setToolTip(tr("No custom icon selected"));
}

void GameConfigDialog::onAcceptClicked()
{
    if (m_titleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter the game title."));
        m_titleEdit->setFocus();
        return;
    }

    if (m_executableEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please select the game executable."));
        m_executableEdit->setFocus();
        return;
    }

    accept();
}

void GameConfigDialog::setGame(const Game &game)
{
    m_gameId = game.id;
    m_titleEdit->setText(game.title);
    m_executableEdit->setText(game.executablePath);
    m_iconEdit->setText(game.iconPath);
    updateIconPreview(game.iconPath);

    int runnerIndex = m_runnerCombo->findData(static_cast<int>(game.runner));
    if (runnerIndex >= 0) {
        m_runnerCombo->setCurrentIndex(runnerIndex);
    }

    m_prefixEdit->setText(game.winePrefix);

    int versionIndex = m_wineVersionCombo->findData(game.wineVersion);
    if (versionIndex < 0) {
        versionIndex = m_wineVersionCombo->findText(game.wineVersion);
    }
    if (versionIndex >= 0) {
        m_wineVersionCombo->setCurrentIndex(versionIndex);
    }

    bool isWine = (game.runner == GameRunner::Wine);
    m_wineGroup->setVisible(isWine);
    m_nativeGroup->setVisible(!isWine);
    m_preCommandEdit->setText(game.preCommand);
    adjustSize();
}

Game GameConfigDialog::game() const
{
    Game g;
    g.id = m_gameId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_gameId;
    g.title = m_titleEdit->text().trimmed();
    g.executablePath = m_executableEdit->text().trimmed();
    g.iconPath = m_iconEdit->text().trimmed();
    g.preCommand = m_preCommandEdit->text().trimmed();
    g.runner = static_cast<GameRunner>(m_runnerCombo->currentData().toInt());
    g.winePrefix = m_prefixEdit->text().trimmed();
    g.wineVersion = m_wineVersionCombo->currentData().toString();
    if (g.wineVersion.isEmpty()) {
        g.wineVersion = m_wineVersionCombo->currentText();
    }
    return g;
}
