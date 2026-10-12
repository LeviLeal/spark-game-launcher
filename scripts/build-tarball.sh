#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
VERSION="0.1.0"
ARCH="$(uname -m)"
PACKAGE_NAME="spark-${VERSION}-linux-${ARCH}"
DIST_DIR="${BUILD_DIR}/dist/${PACKAGE_NAME}"
TARBALL="${BUILD_DIR}/${PACKAGE_NAME}.tar.xz"

echo "==> Compilando o Spark em Release..."
"${PROJECT_DIR}/scripts/build.sh" --release

echo "==> Preparando estrutura do pacote ${PACKAGE_NAME}..."
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}"

# 1. Binário compilado
cp "${BUILD_DIR}/spark" "${DIST_DIR}/spark"
chmod +x "${DIST_DIR}/spark"

# 2. Launcher script para compatibilidade com clique duplo
cat << 'EOF' > "${DIST_DIR}/spark.sh"
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"
exec "${SCRIPT_DIR}/spark" "$@"
EOF
chmod +x "${DIST_DIR}/spark.sh"

# 3. Ícone e template de .desktop
cp "${PROJECT_DIR}/logo.png" "${DIST_DIR}/spark.png"

cat << 'EOF' > "${DIST_DIR}/spark.desktop"
[Desktop Entry]
Type=Application
Name=Spark Game Launcher
GenericName=Game Launcher
Comment=Modern game launcher for Linux with Wine and Proton support
Comment[pt_BR]=Launcher de jogos para Linux com suporte a Proton e Wine
Exec=spark %u
Icon=spark
Terminal=false
Categories=Game;Utility;
StartupWMClass=Spark
EOF

# 4. README com requisitos e instruções
cat << 'EOF' > "${DIST_DIR}/README.md"
# Spark Game Launcher

Launcher de jogos para Linux com suporte integrado a Wine e Proton.

## Como Executar

1. Dê dois cliques em `spark` ou `spark.sh`, ou execute no terminal:
   ```bash
   ./spark
   ```

## Integração com o Menu do Sistema

Abra o Spark, clique no ícone de **Configurações (Settings)** na barra superior e, na aba **Desktop Integration**, clique em **"Adicionar ao Menu (.desktop)"**.
O aplicativo criará automaticamente o atalho no menu de aplicativos do seu sistema apontando para onde esta pasta estiver localizada.

## Requisitos do Sistema

- Linux (x86_64)
- Qt 6 (Widgets e Network)
  - Arch Linux / Manjaro: `sudo pacman -S qt6-base`
  - Ubuntu / Debian: `sudo apt install libqt6widgets6 libqt6network6`
  - Fedora: `sudo dnf install qt6-qtbase-gui`
EOF

echo "==> Compactando em .tar.xz..."
tar -C "${BUILD_DIR}/dist" -cJf "${TARBALL}" "${PACKAGE_NAME}"

echo "==> Pacote gerado com sucesso!"
echo "Localização: ${TARBALL}"
ls -lh "${TARBALL}"
