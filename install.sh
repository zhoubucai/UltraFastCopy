#!/bin/bash
set -e

VERSION="1.2.0"
ARCH="amd64"
REPO="zhoubucai/UltraFastCopy"

echo "正在安装 ufcp v${VERSION}..."

INSTALL_URL="https://github.com/${REPO}/releases/download/v${VERSION}/ufcp-${VERSION}-linux-${ARCH}.tar.gz"

TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

curl -fsSL "$INSTALL_URL" -o ufcp.tar.gz
tar -xzf ufcp.tar.gz

install -Dm755 "ufcp-${VERSION}-linux-${ARCH}/ufcp" /usr/local/bin/ufcp

cd /
rm -rf "$TEMP_DIR"

echo "安装成功！运行 'ufcp --help' 查看用法"