#include "generalsettingswidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QIcon>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFontDatabase>

GeneralSettingsWidget::GeneralSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refreshStatus();
}

void GeneralSettingsWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(18);

    // GroupBox Integração com o Desktop
    auto *group = new QGroupBox(tr("Desktop Integration"), this);
    group->setFlat(true);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setSpacing(14);
    groupLayout->setContentsMargins(16, 18, 16, 16);

    auto *descLabel = new QLabel(
        tr("Register Spark in your application launcher menu (.desktop). "
           "This intelligently points to wherever Spark is currently located on your system."),
        group
    );
    descLabel->setWordWrap(true);
    QFont descFont = descLabel->font();
    descFont.setPointSize(descFont.pointSize() > 0 ? descFont.pointSize() + 1 : 10);
    descLabel->setFont(descFont);
    groupLayout->addWidget(descLabel);

    auto *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setSpacing(10);

    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // Executável atual
    auto *exeRowLayout = new QHBoxLayout();
    m_currentExeEdit = new QLineEdit(group);
    m_currentExeEdit->setReadOnly(true);
    m_currentExeEdit->setFont(monoFont);
    m_currentExeEdit->setText(currentExecutablePath());
    exeRowLayout->addWidget(m_currentExeEdit, 1);

    auto *openDirBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-open")), tr("Open Folder"), group);
    openDirBtn->setToolTip(tr("Open directory containing the current executable"));
    connect(openDirBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(currentExecutablePath()).absolutePath()));
    });
    exeRowLayout->addWidget(openDirBtn);
    formLayout->addRow(tr("Current Executable:"), exeRowLayout);

    // Caminho do .desktop
    m_desktopPathEdit = new QLineEdit(group);
    m_desktopPathEdit->setReadOnly(true);
    m_desktopPathEdit->setFont(monoFont);
    m_desktopPathEdit->setText(desktopFilePath());
    formLayout->addRow(tr("Desktop File Target:"), m_desktopPathEdit);

    groupLayout->addLayout(formLayout);

    // Botões de Ação
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_addOrUpdateBtn = new QPushButton(group);
    m_addOrUpdateBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_addOrUpdateBtn->setStyleSheet(QStringLiteral("font-weight: bold; padding: 6px 14px;"));
    connect(m_addOrUpdateBtn, &QPushButton::clicked, this, &GeneralSettingsWidget::onAddOrUpdateDesktop);
    btnLayout->addWidget(m_addOrUpdateBtn);

    m_removeBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete"),
                                                   QIcon::fromTheme(QStringLiteral("user-trash"))),
                                  tr("Remove Shortcut"), group);
    m_removeBtn->setStyleSheet(QStringLiteral("padding: 6px 14px;"));
    connect(m_removeBtn, &QPushButton::clicked, this, &GeneralSettingsWidget::onRemoveDesktop);
    btnLayout->addWidget(m_removeBtn);

    btnLayout->addStretch(1);
    groupLayout->addLayout(btnLayout);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

QString GeneralSettingsWidget::currentExecutablePath() const
{
    QString exe = QCoreApplication::applicationFilePath();
    QFileInfo fi(exe);
    if (fi.isSymLink()) {
        QString canonical = fi.canonicalFilePath();
        if (!canonical.isEmpty()) {
            return canonical;
        }
    }
    return fi.absoluteFilePath();
}

QString GeneralSettingsWidget::desktopFilePath() const
{
    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (appsDir.isEmpty()) {
        appsDir = QDir::homePath() + QStringLiteral("/.local/share/applications");
    }
    return appsDir + QStringLiteral("/spark.desktop");
}

QString GeneralSettingsWidget::iconFilePath() const
{
    QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (genericData.isEmpty()) {
        genericData = QDir::homePath() + QStringLiteral("/.local/share");
    }
    return genericData + QStringLiteral("/icons/hicolor/512x512/apps/spark.png");
}

QString GeneralSettingsWidget::readExecFromDesktop(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("Exec="))) {
            QString execPart = line.mid(5).trimmed();
            // Remove aspas ou flags como %u
            if (execPart.startsWith(QLatin1Char('"'))) {
                int secondQuote = execPart.indexOf(QLatin1Char('"'), 1);
                if (secondQuote != -1) {
                    return execPart.mid(1, secondQuote - 1);
                }
            }
            QStringList tokens = execPart.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (!tokens.isEmpty()) {
                return tokens.first();
            }
            return execPart;
        }
    }
    return QString();
}

void GeneralSettingsWidget::refreshStatus()
{
    QString targetFile = desktopFilePath();
    QString currentExe = currentExecutablePath();

    m_currentExeEdit->setText(currentExe);
    m_desktopPathEdit->setText(targetFile);

    if (QFile::exists(targetFile)) {
        QString registeredExec = readExecFromDesktop(targetFile);
        QFileInfo curFi(currentExe);
        QFileInfo regFi(registeredExec);

        bool pathsMatch = (curFi.canonicalFilePath() == regFi.canonicalFilePath()) ||
                          (curFi.absoluteFilePath() == regFi.absoluteFilePath());

        if (pathsMatch) {
            m_addOrUpdateBtn->setText(tr("Reinstall Shortcut"));
            m_addOrUpdateBtn->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
            m_addOrUpdateBtn->setToolTip(tr("Shortcut is already installed and points to this executable"));
            m_removeBtn->setEnabled(true);
        } else {
            m_addOrUpdateBtn->setText(tr("Update Shortcut to Current Location"));
            m_addOrUpdateBtn->setIcon(QIcon::fromTheme(QStringLiteral("system-run"),
                                      QIcon::fromTheme(QStringLiteral("view-refresh"))));
            m_addOrUpdateBtn->setToolTip(tr("Shortcut points to: %1").arg(registeredExec.isEmpty() ? tr("(unknown)") : registeredExec));
            m_removeBtn->setEnabled(true);
        }
    } else {
        m_addOrUpdateBtn->setText(tr("Add to System Menu (.desktop)"));
        m_addOrUpdateBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        m_addOrUpdateBtn->setToolTip(tr("Register Spark in your application launcher"));
        m_removeBtn->setEnabled(false);
    }
}

void GeneralSettingsWidget::onAddOrUpdateDesktop()
{
    QString currentExe = currentExecutablePath();
    QString targetDesktop = desktopFilePath();
    QString targetIcon = iconFilePath();

    // 1. Garantir diretórios de destino
    QFileInfo desktopFi(targetDesktop);
    QDir().mkpath(desktopFi.absolutePath());

    QFileInfo iconFi(targetIcon);
    QDir().mkpath(iconFi.absolutePath());

    // 2. Extrair ícone embutido (:/logo.png) para os caminhos XDG
    QFile embeddedIcon(QStringLiteral(":/logo.png"));
    if (embeddedIcon.open(QIODevice::ReadOnly)) {
        QByteArray iconData = embeddedIcon.readAll();
        embeddedIcon.close();

        QFile outFile(targetIcon);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(iconData);
            outFile.close();
        }

        // Cópia adicional de fallback em ~/.local/share/icons/spark.png
        QString fallbackIcon = QFileInfo(targetIcon).dir().absolutePath() + QStringLiteral("/../../spark.png");
        QDir().mkpath(QFileInfo(fallbackIcon).absolutePath());
        QFile fallbackOut(fallbackIcon);
        if (fallbackOut.open(QIODevice::WriteOnly)) {
            fallbackOut.write(iconData);
            fallbackOut.close();
        }
    }

    // 3. Montar o conteúdo do .desktop
    QString desktopContent = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Spark Game Launcher\n"
        "GenericName=Game Launcher\n"
        "Comment=Modern game launcher for Linux with Wine and Proton support\n"
        "Comment[pt_BR]=Launcher de jogos para Linux com suporte a Proton e Wine\n"
        "Exec=\"%1\" %u\n"
        "Icon=spark\n"
        "Terminal=false\n"
        "Categories=Game;Utility;\n"
        "StartupWMClass=Spark\n"
    ).arg(currentExe);

    // 4. Gravar arquivo
    QFile file(targetDesktop);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to write desktop entry to:\n%1\n\nError: %2")
                              .arg(targetDesktop, file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << desktopContent;
    file.close();

    // Dar permissões de leitura/execução
    QFile::setPermissions(targetDesktop,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                          QFileDevice::ReadGroup | QFileDevice::ReadOther);

    runDesktopDatabaseUpdate();
    refreshStatus();

    QMessageBox::information(this, tr("Success"),
                             tr("Spark shortcut successfully registered in your application menu!\n\n"
                                "Target: %1\nExecutable: %2")
                             .arg(targetDesktop, currentExe));
}

void GeneralSettingsWidget::onRemoveDesktop()
{
    QString targetDesktop = desktopFilePath();
    if (!QFile::exists(targetDesktop)) {
        return;
    }

    auto reply = QMessageBox::question(
        this,
        tr("Confirm Removal"),
        tr("Are you sure you want to remove the Spark application shortcut from your system menu?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    if (QFile::remove(targetDesktop)) {
        runDesktopDatabaseUpdate();
        refreshStatus();
        QMessageBox::information(this, tr("Removed"),
                                 tr("Shortcut successfully removed from your application menu."));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to remove shortcut file:\n%1").arg(targetDesktop));
    }
}

void GeneralSettingsWidget::runDesktopDatabaseUpdate()
{
    QString appsDir = QFileInfo(desktopFilePath()).absolutePath();
    // Tenta atualizar desktop database se o utilitário existir
    QProcess::startDetached(QStringLiteral("update-desktop-database"), QStringList() << appsDir);
}
