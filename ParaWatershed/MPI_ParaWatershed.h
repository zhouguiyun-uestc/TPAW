#pragma once
#include <filesystem>
#include <map>
#include <Grid/cell.h>
#include "grid_info.h"


void  mainProcess(
	int totalProcessCount,
	const std::filesystem::path& dirTileFolder,
	int tileCount,
	GridInfo& gridInfo);
void computeProcess(int rank, int totalProcessCount, const std::filesystem::path& dirTileFolder,
	const std::filesystem::path& wsTileFolder, std::vector<std::filesystem::path>& allTileFiles, 
	GridInfo& gridInfo,
	std::map<Cell, int>& globalOutlets);

int mpi_main(int argc, char* argv[]);

