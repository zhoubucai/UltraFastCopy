#!/bin/bash
set -e

VERSION="1.2.0"
ARCH="amd64"

echo "构建 ufcp v${VERSION}..."

make clean
make

PKG_DIR="release/ufcp-${VERSION}-linux-${ARCH}"
mkdir -p "$PKG_DIR"
cp bin/ufcp "$PKG_DIR/"

tar -czvf "release/ufcp-${VERSION}-linux-${ARCH}.tar.gz" -C release "ufcp-${VERSION}-linux-${ARCH}"
rm -rf "$PKG_DIR"

echo "构建完成！"
ls -lh release/