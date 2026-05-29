#include "../include/UltraFastCopy.h"
#include "../include/Utils.h"
#include "../include/Logger.h"
#include <filesystem>
#include <system_error>
#include <thread>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <cstdlib>
#include <chrono>
#include <iostream>

extern Logger g_log;

// -------------------------- 构造/析构函数实现 --------------------------
//构造函数
UltraFastCopy::UltraFastCopy() noexcept : _blocks(nullptr), _totalCopiedSize(0), _isCopyDone(false){}
//析构函数
UltraFastCopy::~UltraFastCopy() noexcept {
    if(_blocks){
        for (auto& e : *_blocks) delete e;
	    delete _blocks;
    }
}

// -------------------------- 公有函数实现 --------------------------
//开始拷贝
void UltraFastCopy::Start(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept{
	if(Utils::IsRotationalDisk(sourceFilePath) || Utils::IsRotationalDisk(destinationDirectoryPath)){
		SingleThreadCopy(sourceFilePath,destinationDirectoryPath);
	}
	else{
		MultiThreadCopy(sourceFilePath, destinationDirectoryPath);
	}
}

// -------------------------- 私有函数实现 --------------------------
//判断是否为普通文件且文件存在
bool UltraFastCopy::IsRegularFileExist(const std::string& filePath) noexcept{
	std::ifstream file(filePath, std::ios::binary);
	return file.is_open();
}
//获取文件大小
uint64_t UltraFastCopy::GetFileSize(const std::string& filePath) {
	if (!IsRegularFileExist(filePath)) throw std::runtime_error("文件不存在/非普通文件: " + filePath);
	std::ifstream file(filePath, std::ios::binary);
	file.seekg(0, std::ios::end);
	if (file.fail()) throw std::runtime_error("定位文件末尾失败: " + filePath);
	std::streampos pos = file.tellg();
	if (pos == std::streampos(-1)) throw std::runtime_error("获取文件大小失败: " + filePath);
	if (static_cast<uint64_t>(pos) > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("文件大小≥ 2^34 GiB" + filePath);
	return static_cast<uint64_t>(pos);
}
//计算当前系统最优读写大小，即单次read/write的大小
uint64_t UltraFastCopy::CalculateOptimalIOSize(bool isRotationalDisk) noexcept{
	//现代 NVMe/SSD 高速存储设备的顺序读写甜点通常在 1MB - 4MB 之间
    //采用 4MB 缓冲区，极大减少系统调用(Context Switch)次数
	//注：4MB既是单次IO大小，也是线程缓冲区的大小，最多16线程，则最多会预分配64MB的内存
	//todo：当前 4MB 为经验值，未来需通过 OS API 动态获取底层文件系统/块设备的最优 I/O 大小
	if(isRotationalDisk) return 1 * 1024 * 1024;//1MB
	return 4 * 1024 * 1024;//4MB
}
//计算当前系统最优的块数量，即文件分块数量
uint64_t UltraFastCopy::CalculateBlockNum(uint64_t fileSize) noexcept{
	const uint64_t MIN_BLOCK_SIZE = 32 * 1024 * 1024;//最小处理阈值，如果文件大小低于32MB则不分块，减少线程创建的开销（todo，32MB并非严格测试后的最优值，而是暂定值）
	if(fileSize < MIN_BLOCK_SIZE) return 1;
	uint64_t suggestedByFileSize = fileSize / MIN_BLOCK_SIZE;//根据文件大小计算出分块数量
	uint64_t cpuCore = std::thread::hardware_concurrency();//获取系统cpu核心数
	const uint64_t MAX_IO_THREADS = 16;//线程数上限（todo，暂定值，不同存储介质该值不同，例如机械硬盘最合适的线程数是1）
	return std::min({suggestedByFileSize,cpuCore,MAX_IO_THREADS});//取最小值
}
//切割文件
void UltraFastCopy::SplitFile(const std::string& filePath) noexcept{
	LOG_DEBUG("开始文件分块: " + filePath);
    try{
        uint64_t fileSize = GetFileSize(filePath);
        if (fileSize == 0){
			LOG_ERROR("源文件大小为0，无法分块: " + filePath);
			return;
		}
        else _blocks=new std::vector<FileBlock*>;
        uint64_t blockNum = CalculateBlockNum(fileSize);
	    uint64_t blockSize = fileSize / blockNum;
	    uint64_t remainSize = fileSize % blockNum;//剩余字节数，最后一个块处理
        for (uint64_t i = 0; i < blockNum; ++i) {
            FileBlock* fileBlock = new FileBlock;
            fileBlock->_startOffset = i * blockSize;
            if (i == blockNum - 1) fileBlock->_blockSize = blockSize + remainSize;
            else fileBlock->_blockSize = blockSize;
            _blocks->push_back(fileBlock);
        }
        LOG_DEBUG("文件分块完成: " + filePath);
    }catch(const std::exception& e){
        LOG_ERROR(e.what());
        LOG_ERROR("文件分块失败: " + filePath);
        std::exit(1);
    }
}
//线程函数：拷贝文件块
void UltraFastCopy::CopyBlock(const std::string& sourceFilePath, const std::string& destinationFilePath, FileBlock* fileBlock, int threadID) {
	//打开源文件
	std::ifstream sourceFile(sourceFilePath, std::ios::binary);
	if (!sourceFile.is_open()) throw std::runtime_error("线程" + std::to_string(threadID) + ": 文件不存在/非普通文件: " + sourceFilePath);
	//定位到源文件的块起始偏移
	sourceFile.seekg(fileBlock->_startOffset, std::ios::beg);
	if (sourceFile.fail()) throw std::runtime_error("线程" + std::to_string(threadID) + "定位文件失败: " + sourceFilePath);
	//打开目标文件
	std::ofstream destinationFile(destinationFilePath, std::ios::binary | std::ios::in |std::ios::out);
	if (!destinationFile.is_open()) throw std::runtime_error("线程" + std::to_string(threadID) + ": 文件不存在/非普通文件: " + destinationFilePath);
	//定位到目标文件的块起始偏移
	destinationFile.seekp(fileBlock->_startOffset, std::ios::beg);
	if (destinationFile.fail()) throw std::runtime_error("线程" + std::to_string(threadID) + "定位文件失败: " + destinationFilePath);
	//创建线程缓冲区（预分配内存，避免动态扩容）
	uint64_t optimalIOSize = CalculateOptimalIOSize();//单次最优读写大小
	std::vector<char> buffer(optimalIOSize);
	//读写数据（边读边写）
	uint64_t copiedSize = 0;//已拷贝的数据大小
	while (copiedSize < fileBlock->_blockSize) {
		uint64_t toRead=std::min(optimalIOSize,fileBlock->_blockSize-copiedSize);//每次读取的字节数不能超过块剩余大小
		sourceFile.read(buffer.data(), toRead);
		if (sourceFile.fail()) throw std::runtime_error("线程" + std::to_string(threadID) + "读取数据失败: " + sourceFilePath);
		uint64_t readSize = sourceFile.gcount();//本次实际读取到的数据大小
		destinationFile.write(buffer.data(), readSize);
		if (destinationFile.fail()) throw std::runtime_error("线程" + std::to_string(threadID) + "写入数据失败: " + destinationFilePath);
		copiedSize += readSize;
		_totalCopiedSize += readSize;
		LOG_DEBUG("线程" + std::to_string(threadID) + "本次拷贝" + std::to_string(toRead) + "字节，已拷贝 " + std::to_string(copiedSize) + " / " + std::to_string(fileBlock->_blockSize) + " 字节");
	}
}
//多线程拷贝
void UltraFastCopy::MultiThreadCopy(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept{
	LOG_DEBUG("开始多线程拷贝文件: From " + sourceFilePath + " to " + destinationDirectoryPath);
	//分块
	SplitFile(sourceFilePath);
	//判断
	if (_blocks == nullptr) return;
	//创建线程容器
	std::vector<std::thread> copyThreads;
    //构建目标文件路径
    std::string destinationFilePath=destinationDirectoryPath+"/"+Utils::GetFileName(sourceFilePath);
	//检查目标文件目录是否存在，不存在则创建
	std::filesystem::path destDir(destinationDirectoryPath);
	if(!std::filesystem::exists(destDir)){
		std::error_code ec;
		if(std::filesystem::create_directories(destDir,ec)){
			LOG_INFO("目标目录不存在，已自动级联创建: " + destDir.string());
		}
		else if(ec){
			LOG_ERROR("目标目录不存在，且级联创建失败: " + destDir.string() + ", 错误信息: " + ec.message());
			std::exit(1);
		}
	}
	std::ofstream destinationFile(destinationFilePath, std::ios::binary | std::ios::out);
	if(!destinationFile.is_open()) {
		LOG_ERROR("创建目标文件失败: " + destinationFilePath);
		std::exit(1);
	}
	//预分配目标文件大小
	uint64_t fileSize=GetFileSize(sourceFilePath);
	destinationFile.seekp(fileSize - 1);//定位到文件末尾
	destinationFile.write("", 1);//写入一个字节以扩展文件大小
	if(destinationFile.fail()){
		LOG_ERROR("预分配目标文件大小失败: " + destinationFilePath);
		std::exit(1);
	}
	destinationFile.close();
	//启动监控线程
	StartMonitor(fileSize);
	//启动线程拷贝各个块
	try{
		int threadNum=_blocks->size();//数据块数量即线程数量
		//初始化线程容器
		for (int i = 0; i < threadNum; ++i) copyThreads.emplace_back(&UltraFastCopy::CopyBlock, this, sourceFilePath, destinationFilePath, (*_blocks)[i], i);
		//等待所有线程运行完毕
		for (auto& copyThread : copyThreads) {
			copyThread.join();
		}
		StopMonitor();
		LOG_DEBUG("文件拷贝完成: From " + sourceFilePath + " to " + destinationFilePath);
	}catch(const std::exception& e){
		StopMonitor();
		LOG_ERROR(e.what());
		LOG_ERROR("文件拷贝失败: From " + sourceFilePath + " to " + destinationFilePath);
		std::exit(1);
	}
}
//单线程拷贝
void UltraFastCopy::SingleThreadCopy(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept{
	try{
		LOG_DEBUG("开始单线程拷贝文件: From " + sourceFilePath + " to " + destinationDirectoryPath);
		//构建目标文件路径
		std::string destinationFilePath=destinationDirectoryPath+"/"+Utils::GetFileName(sourceFilePath);
		//检查目标文件目录是否存在，不存在则创建
		std::filesystem::path destDir(destinationDirectoryPath);
		if(!std::filesystem::exists(destDir)){
			std::error_code ec;
			if(std::filesystem::create_directories(destDir,ec)){
				LOG_INFO("目标目录不存在，已自动级联创建: " + destDir.string());
			}
			else if(ec){
				LOG_ERROR("目标目录不存在，且级联创建失败: " + destDir.string() + ", 错误信息: " + ec.message());
				std::exit(1);
			}
		}
		//创建目标文件
		std::ofstream destinationFile(destinationFilePath, std::ios::binary | std::ios::out);
		if(!destinationFile.is_open()) {
			LOG_ERROR("创建目标文件失败: " + destinationFilePath);
			std::exit(1);
		}
		//预分配目标文件大小
		uint64_t fileSize=GetFileSize(sourceFilePath);
		destinationFile.seekp(fileSize - 1);//定位到文件末尾
		destinationFile.write("", 1);//写入一个字节以扩展文件大小
		if(destinationFile.fail()){
			LOG_ERROR("预分配目标文件大小失败: " + destinationFilePath);
			std::exit(1);
		}
		//重置目标文件写指针
		destinationFile.seekp(0);
		//创建源文件对象
		std::ifstream sourceFile(sourceFilePath);
		if(!sourceFile.is_open()){
			LOG_ERROR("打开源文件失败: "+sourceFilePath);
			std::exit(1);
		}
		//创建线程缓冲区（预分配内存，避免动态扩容）
		uint64_t optimalIOSize = CalculateOptimalIOSize(true);//单次最优读写大小
		std::vector<char> buffer(optimalIOSize);
		//启动监控线程
		StartMonitor(fileSize);
		//拷贝
		uint64_t copiedSize=0;
		while(copiedSize<fileSize){
			uint64_t toRead=std::min(fileSize-copiedSize,optimalIOSize);//每次读取的字节数不能超过块剩余大小
			sourceFile.read(buffer.data(),toRead);
			if(sourceFile.fail()){
				LOG_ERROR("读取数据失败: "+sourceFilePath);
				std::exit(1);
			}
			uint64_t readSize=sourceFile.gcount();//实际读取到的数据
			destinationFile.write(buffer.data(),readSize);
			if(destinationFile.fail()){
				LOG_ERROR("写入数据失败: "+destinationFilePath);
				std::exit(1);
			}
			copiedSize+=readSize;
			_totalCopiedSize+=readSize;
			LOG_DEBUG("本次已拷贝: "+std::to_string(toRead) + "字节，"+ std::to_string(copiedSize) + " / " + std::to_string(fileSize) + " 字节");
		}
		//停止监控线程
		StopMonitor();
	}catch(const std::exception& e){
		StopMonitor();
		LOG_ERROR("单线程拷贝异常: "+std::string(e.what()));
		std::exit(1);
	}catch(...){
		StopMonitor();
		LOG_ERROR("单线程拷贝发生未知异常");
		std::exit(1);
	}
}
//开启监控线程
void UltraFastCopy::StartMonitor(uint64_t fileSize) noexcept{
	//重置拷贝大小和状态
	_totalCopiedSize=0; 
    _isCopyDone=false;
	//记录开始时间
	_startTime=std::chrono::high_resolution_clock::now();
	//创建监控线程
	_monitorThread=std::thread([this,fileSize](){
		uint64_t lastCopiedSize=_totalCopiedSize.load();
		auto lastTime=std::chrono::high_resolution_clock::now();
		while(!_isCopyDone){
			std::this_thread::sleep_for(std::chrono::milliseconds(200));//每隔200毫秒刷新一次，同时释放cpu给核心拷贝线程
			uint64_t currentCopiedSize=_totalCopiedSize.load();
			auto currentTime=std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed=currentTime-lastTime;
			double speedMBps=0.0;//瞬时速度
            if (currentCopiedSize > lastCopiedSize && elapsed.count() > 0) {
                speedMBps=static_cast<double>(currentCopiedSize-lastCopiedSize)/1024.0/1024.0/elapsed.count();
            }
			double progress=fileSize>0?static_cast<double>(currentCopiedSize)/fileSize*100.0:100.0;
			std::cout << "\r\033[36m[监控]\033[0m 进度: " 
                  << std::fixed << std::setprecision(1) << std::setw(5) << progress << "% | "
                  << "速度: " << std::setw(8) << speedMBps << " MB/s | "
                  << currentCopiedSize / 1024 / 1024 << " / " << fileSize / 1024 / 1024 << " MB " 
                  << std::flush;
			//更新拷贝数据
			lastCopiedSize=currentCopiedSize;
            lastTime=currentTime;
		}
	});
}
//停止监控线程
void UltraFastCopy::StopMonitor() noexcept{
	if(!_monitorThread.joinable()) return;//监控线程已经结束，或者没开始运行
	_isCopyDone=true;//设置拷贝完成标志
	_monitorThread.join();
	std::cout<<"\n";//确保进度条不被后续日志输出干扰
	//计算总拷贝耗时
	auto endTime=std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> totalElapsed=endTime-_startTime;
	LOG_INFO("✅ 性能统计: 总耗时 " + std::to_string(totalElapsed.count()) + " 秒");
}