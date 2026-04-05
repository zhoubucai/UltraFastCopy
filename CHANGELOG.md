# Changelog

本项目的所有重要变更都会记录在此文件中。
格式标准参考 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)。

## [Unreleased]

### 🚀 性能优化 (Performance)
* **动态分块计算**：重构了分块数量的算法，摒弃硬编码。现支持基于目标文件大小（32MB阈值）与主机 CPU 逻辑核心数的自适应并发，消除了极小文件场景下的线程调度损耗，并在大文件场景下更科学地压榨硬件性能。

### 📝 工程规范 (Documentation)
* **架构演进**：新增 `docs/Architecture.md`，正式确立 V1.0.0 的“静态多线程分块”核心架构与技术背景。
* **提交模板**：引入 `.gitmessage` 规范，强制在每次底层优化时复盘问题背景、决策依据与优缺点分析。

---

## [v1.0.0] - 2026-01-26

### 🎉 初始版本
* 实现了 `UltraFastCopy` 的基本功能。