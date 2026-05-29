# UltraFastCopy (ufcp)

高性能 Linux 文件复制工具，支持 SSD 多线程并发拷贝与 HDD 单线程顺序拷贝。

## 功能特性

- **存储介质感知**：自动检测源/目标所在块设备类型（SSD/HDD）
- **多线程并发（SSD）**：基于文件大小与 CPU 核心数动态分块，多线程并行写入
- **单线程顺序拷贝（HDD）**：机械硬盘场景自动切换单线程模式，避免磁头寻道损耗
- **动态 I/O 缓冲**：SSD 使用 4MB 缓冲区，HDD 使用 1MB 缓冲区
- **实时进度监控**：无锁监控面板，实时显示拷贝进度与速度

## 安装

### 一键安装

```bash
curl -fsSL https://raw.githubusercontent.com/zhoubucai/UltraFastCopy/v1.2.0/install.sh | sh
```

### 源码编译安装

```bash
git clone https://github.com/zhoubucai/UltraFastCopy.git
cd UltraFastCopy
make && sudo make install
```

## 使用

```bash
ufcp --help                          # 查看帮助
ufcp --version                       # 查看版本
ufcp /path/to/source/file /path/to/dest/    # 拷贝文件到目标目录
```

## 要求

- Linux 操作系统（x86_64 / amd64）
- g++ 支持 C++17
- pthread 库

## 许可

MIT License