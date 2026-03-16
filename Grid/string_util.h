#pragma once
#include <vector>
#include <string>
#include <filesystem>
std::vector<std::string> split(const std::string& str, char delimiter);
std::string trim(const std::string& str);
std::vector<std::string> readAlllines(const std::filesystem::path& path);