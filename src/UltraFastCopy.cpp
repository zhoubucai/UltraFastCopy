#include "../include/UltraFastCopy.h"
#include "../include/Utils.h"
#include "../include/Logger.h"
#include <filesystem>
#include <system_error>

extern Logger g_log;

// -------------------------- 构造/析构函数实现 --------------------------
//构造函数
UltraFastCopy::UltraFastCopy() noexcept : _blocks(nullptr) {}
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
	MultiThreadCopy(sourceFilePath, destinationDirectoryPath);
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
	if (pos > std::numeric_limits<uint64_t>::max()) throw std::overflow_error("文件大小≥ 2^34 GiB" + filePath);
	return static_cast<uint64_t>(pos);
}
//计算当前系统最优读写大小，即单次read/write的大小
uint64_t UltraFastCopy::CalculateOptimalIOSize() noexcept{
	//现代 NVMe/SSD 高速存储设备的顺序读写甜点通常在 1MB - 4MB 之间
    //采用 4MB 缓冲区，极大减少系统调用(Context Switch)次数
	//注：4MB既是单次IO大小，也是线程缓冲区的大小，最多16线程，则最多会预分配64MB的内存
	//todo：当前 4MB 为经验值，未来需通过 OS API 动态获取底层文件系统/块设备的最优 I/O 大小
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
	LOG_INFO("开始文件分块: " + filePath);
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
        LOG_INFO("文件分块完成: " + filePath);
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
		LOG_INFO("线程" + std::to_string(threadID) + "本次拷贝" + std::to_string(toRead) + "字节，已拷贝 " + std::to_string(copiedSize) + " / " + std::to_string(fileBlock->_blockSize) + " 字节");
	}
}
//多线程拷贝
void UltraFastCopy::MultiThreadCopy(const std::string& sourceFilePath, const std::string& destinationDirectoryPath) noexcept{
	LOG_INFO("开始多线程拷贝文件: From " + sourceFilePath + " to " + destinationDirectoryPath);
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
	//启动线程拷贝各个块
	try{
		int threadNum=_blocks->size();//数据块数量即线程数量
		//初始化线程容器
		for (int i = 0; i < threadNum; ++i) copyThreads.emplace_back(&UltraFastCopy::CopyBlock, this, sourceFilePath, destinationFilePath, (*_blocks)[i], i);
		//等待所有线程运行完毕
		for (auto& copyThread : copyThreads) {
			copyThread.join();
		}
		LOG_INFO("文件拷贝完成: From " + sourceFilePath + " to " + destinationFilePath);
	}catch(const std::exception& e){
		LOG_ERROR(e.what());
		LOG_ERROR("文件拷贝失败: From " + sourceFilePath + " to " + destinationFilePath);
		std::exit(1);
	}
}