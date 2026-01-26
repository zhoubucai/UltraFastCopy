#include "../include/Utils.h"
#include <algorithm>

namespace Utils{
    //获取文件名
    std::string GetFileName(const std::string& filePath) noexcept{
        //支持Linux/Windows
        auto rit=find(filePath.rbegin(),filePath.rend(),'/');
        if(rit==filePath.rend()) return filePath;
        if(rit.base()==filePath.end()) return "";//filePath的最后一个字符是'/'
        else return std::string(rit.base(),filePath.end());
    }
}