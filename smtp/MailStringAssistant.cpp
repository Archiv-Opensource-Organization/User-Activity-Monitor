#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

std::string openFiletoString(const std::string& file_path) {
    std::ifstream fin(file_path);
    if (!fin.is_open())
    {
        throw std::runtime_error("无法打开模板文件: " + file_path);
    }
    std::stringstream buf;
    buf << fin.rdbuf();
    return buf.str();
}

void replaceAll(std::string& source, const std::string& key, const std::string& replacement) {
    size_t pos = 0;
    while ((pos = source.find(key, pos)) != std::string::npos)
    {
        source.replace(pos, key.size(), replacement);
        pos += replacement.size();
    }
}