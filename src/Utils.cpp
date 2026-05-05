#include "../include/Utils.h"
#include "../include/Logger.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fstream>

extern Logger g_log;

namespace Utils{
    //获取文件名
    std::string GetFileName(const std::string& filePath) noexcept{
        //支持Linux/Windows
        auto rit=find(filePath.rbegin(),filePath.rend(),'/');
        if(rit==filePath.rend()) return filePath;
        if(rit.base()==filePath.end()) return "";//filePath的最后一个字符是'/'
        else return std::string(rit.base(),filePath.end());
    }
    //判断某个路径的存储介质是否为机械硬盘
    bool IsRotationalDisk(const std::string& path) noexcept{
        try{
            struct stat fileStat;//文件属性结构体
            if(stat(path.c_str(),&fileStat)!=0){
                LOG_DEBUG("获取文件属性失败，默认当作机械硬盘处理: "+path);
                return true;//获取文件属性失败，默认当作机械硬盘处理
            }
            dev_t majorDev=major(fileStat.st_dev);//主设备号
            dev_t minorDev=minor(fileStat.st_dev);//次设备号
            //构建sysfs设备目录路径
            std::string sysfsBase="/sys/dev/block/"+std::to_string(majorDev)+":"+std::to_string(minorDev);
            //构建检测路径，有两种情况（裸盘挂载/dev/sda和分区挂载/dev/sda1）
            std::string detectSysfsPath[2]={sysfsBase+"/queue/rotational",sysfsBase+"/../queue/rotational"};
            //通过 queue/rotational 判断是否为机型硬盘
            for(const std::string& rotationalPath:detectSysfsPath){
                std::ifstream rotationalFile(rotationalPath);
                if(rotationalFile.is_open()){
                    char flag;
                    rotationalFile>>flag;
                    if(flag=='1') return true;
                    else return false;
                }
                LOG_DEBUG("打开文件失败: "+rotationalPath);
            }
            //两个检测路径都打开失败，保守策略，当作机械硬盘处理
            return true;
        }catch(...){
            return true;
        }
    }
}