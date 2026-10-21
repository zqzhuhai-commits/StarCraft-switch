#pragma once
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace starcraft {
inline bool is_map_file(std::string name) {
    auto dot = name.rfind('.');
    if (dot == std::string::npos) return false;
    name = name.substr(dot);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    return name == ".scm" || name == ".scx" || name == ".scc";
}
inline void collect_maps(const std::string& dir, std::vector<std::string>& out, int depth) {
    if (depth > 16) return;
    DIR* handle = opendir(dir.c_str());
    if (!handle) return;
    while (auto* entry = readdir(handle)) {
        std::string name = entry->d_name;
        if (name.empty() || name[0] == '.') continue;
        std::string path = dir + "/" + name;
        struct stat info{};
        if (lstat(path.c_str(), &info) != 0) continue;
        if (S_ISDIR(info.st_mode)) collect_maps(path, out, depth + 1);
        else if (S_ISREG(info.st_mode) && is_map_file(name)) out.push_back(path);
    }
    closedir(handle);
}
inline std::vector<std::string> find_maps(const std::string& dir) {
    std::vector<std::string> out;
    collect_maps(dir, out, 0);
    std::sort(out.begin(), out.end());
    return out;
}
}
