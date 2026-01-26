#include "../include/UltraFastCopy.h"
#include "../include/Logger.h"

Logger g_log("logs/copy.log");//全局日志对象

int main() {
	UltraFastCopy copy;
	copy.Start("/home/zbc/Document/test.txt", "/home/zbc/Code/UltraFastCopy");
	return 0;
}