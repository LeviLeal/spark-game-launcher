#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BUILD_TYPE="Release"
CLEAN_BUILD=false

for arg in "$@"; do
    case "$arg" in
        --debug|-d)
            BUILD_TYPE="Debug"
            ;;
        --release|-r)
            BUILD_TYPE="Release"
            ;;
        --clean|-c)
            CLEAN_BUILD=true
            ;;
        --help|-h)
            echo "Uso: $0 [OPÇÕES]"
            echo "Opções:"
            echo "  --release, -r   Compilar em modo Release (padrão)"
            echo "  --debug,   -d   Compilar em modo Debug"
            echo "  --clean,   -c   Limpar pasta de build antes de compilar"
            echo "  --help,    -h   Exibir esta ajuda"
            exit 0
            ;;
        *)
            echo "Opção desconhecida: $arg"
            echo "Execute '$0 --help' para ver as opções disponíveis."
            exit 1
            ;;
    esac
done

if [ "$CLEAN_BUILD" = true ]; then
    echo "==> Limpando diretório de build: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

echo "==> Configurando CMake (${BUILD_TYPE})..."
cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Compilando Spark (${BUILD_TYPE}) com $(nproc) threads..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j"$(nproc)"

echo "==> Compilação concluída com sucesso!"
echo "Executável gerado: ${BUILD_DIR}/spark"
ls -lh "${BUILD_DIR}/spark"
