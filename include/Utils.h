#pragma once
#include <string>

namespace Utils{
    //获取文件名
    std::string GetFileName(const std::string& filePath) noexcept;
    //判断某个路径的存储介质是否为机械硬盘
    bool IsRotationalDisk(const std::string& path) noexcept;
}