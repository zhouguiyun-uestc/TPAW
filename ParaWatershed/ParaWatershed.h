#pragma once
#include <filesystem>
#include <map>
#include <Grid/cell.h>
#include "retention.h"
using namespace std::filesystem;
void tiled_ws_serial(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets, Retention retention = Retention::Evict);
void tiled_ws_openmp(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets, Retention retention = Retention::Evict);
std::map<Cell, int> getGlobalOutlet(const std::filesystem::path& flowDirPath);

