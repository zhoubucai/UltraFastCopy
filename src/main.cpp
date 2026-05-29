#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include "../include/UltraFastCopy.h"
#include "../include/Logger.h"

Logger g_log;//全局日志对象

void PrintHelp(const std::string& prog_name){
	//提取程序名，去除路径
	std::string prog(prog_name);
	size_t last_slash=prog_name.find_last_of('/');
	if(last_slash!=std::string::npos){
		prog=prog.substr(last_slash+1);
	}

	std::cout<<"用法: "<<prog<<" [选项] <源文件> <目标路径>\n\n"
			 <<"选项:\n"
			 <<"  -h, --help           显示帮助信息\n"
			 <<"  -v, --version        显示版本信息\n\n"
			 <<"描述:\n"
			 <<"高性能文件拷贝工具，支持 SSD 多线程并发拷贝与 HDD 单线程顺序拷贝\n\n"
			 <<"示例:\n"
			 <<prog<<" /path/to/file.txt "<<"/path/to/dest/\n";
}

void PrintVersion(){
	std::cout<<"ufcp (UltraFastCopy) 1.2.0"<<std::endl;
}

int main(int argc, char* argv[]) {
	int opt;//存储getopt_long的返回值
	struct option long_options[]={
		{"help",no_argument,NULL,'h'},
		{"version",no_argument,NULL,'v'},
		{0,0,0,0}
	};

	while((opt=getopt_long(argc,argv,"hv",long_options,NULL))!=-1){
		switch(opt){
			case 'h':
				PrintHelp(argv[0]);
				return 0;
			case 'v':
				PrintVersion();
				return 0;
			default:
				std::cerr<<"请使用 "<<argv[0]<<" -h 或 --help 查看帮助\n";
				return 1;
		}
	}

	if(argc-optind!=2){
		std::cerr<<"请指定源文件与目标路径\n";
		std::cerr<<"请使用 "<<argv[0]<<" -h 或 --help 查看帮助\n";
		return 1;
	}

	std::string src=argv[optind];
	std::string dest=argv[optind+1];

	UltraFastCopy copy;
	copy.Start(src, dest);
	return 0;
}