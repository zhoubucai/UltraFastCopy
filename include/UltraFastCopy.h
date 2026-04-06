#pragma once
#include <cstdint>
#include <string>
#include <vector>

//分块并非真的将文件切割，而是记录文件块的起始位置和块大小
//支持拷贝的文件最大是 2^64-1 字节 ，约2^34GiB
struct FileBlock {
	uint64_t _startOffset;
	uint64_t _blockSize;
	FileBlock() :_startOffset(0), _blockSize(0) {}
};

class UltraFastCopy {
private:
	std::vector<FileBlock*>* _blocks;
public:
	//构造函数
	UltraFastCopy() noexcept;
	//析构函数
	~UltraFastCopy() noexcept;
	//开始拷贝
	void Start(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept;
private:
	//判断是否为普通文件且文件存在
	bool IsRegularFileExist(const std::string& filePath) noexcept;
	//获取文件大小
	uint64_t GetFileSize(const std::string& filePath);
	//计算当前系统最优读写大小，即单次read/write的大小
	uint64_t CalculateOptimalIOSize() noexcept;
	//计算当前系统最优的块数量
	uint64_t CalculateBlockNum(uint64_t fileSize) noexcept;
	//切割文件
	void SplitFile(const std::string& filePath) noexcept;
	//线程函数：拷贝文件块
	void CopyBlock(const std::string& sourceFilePath, const std::string& destinationFilePath, FileBlock* fileBlock, int threadID);
	//多线程拷贝
	void MultiThreadCopy(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept;
};