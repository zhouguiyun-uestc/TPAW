#pragma once
#include <filesystem>
#include <map>
#include <Grid/cell.h>
using namespace std::filesystem;
void tiled_ws_serial(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets);
void tiled_ws_serial1(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets);
void tiled_ws_openmp(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets);
void tiled_ws_openmp1(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets);
std::map<Cell, int> getGlobalOutlet(const std::filesystem::path& flowDirPath);

