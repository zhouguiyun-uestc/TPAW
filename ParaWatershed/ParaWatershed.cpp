#include "ParaWatershed.h"
#include "SolveLocal.h"
#include "SolveGlobal.h"
#include <Grid/io_gdal.h>
#include "tool.h"
#include <Grid/tool.h>
#include <Grid/util.h>
#include <omp.h>

std::map<Cell, int> getGlobalOutlet(const std::filesystem::path& flowDirPath)
{
	Grid<FlowDir> dirGrid = readRaster<FlowDir>(flowDirPath);
	std::map<Cell, int> outlets;
	int wsIndex = 1;
	for (int row = 0; row < dirGrid.height(); row++)
	{
		for (int col = 0; col < dirGrid.width(); col++)
		{
			Cell c(row, col);
			if (!dirGrid.isNoData(c))
			{
				if (!moveToDownstreamCell(dirGrid, c)) {
					//wsGrid(c) = wsIndex++;
					outlets[c]= wsIndex++;
				}
			}
		}
	}

	return outlets;
}

void tiled_ws_serial(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets, Retention retention)
{
	GridInfo gridInfo;
	//read gridInfo
	gridInfo.read(dirFileFolder / "gridInfo.txt");
	std::vector<path> allTileFiles=readAllTileFiles(dirFileFolder / "tiles.txt");
	TileFlowStore store(retention);

	Grid<LocalSolution> gridLocalSolutions(gridInfo.gridHeight, gridInfo.gridWidth);
	gridLocalSolutions.allocate();

	for(int i=0; i<allTileFiles.size(); i++)
	{
			Cell gridCell=Cell::fromString(allTileFiles[i].stem().string());
			path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
			if (!exists(tilePath)) continue;

			Grid<FlowDir> dirGrid = readRaster<FlowDir>(tilePath);
			//set global outlet
			Grid<int> wsGrid(dirGrid);
			wsGrid.allocate();
			std::map <Cell, int> interiorLocalOutlets;
			for (auto& [cell, index] : outlets) {
				auto localCell = cell - Cell(gridCell.row * gridInfo.tileHeight, gridCell.col * gridInfo.tileWidth);
				if (wsGrid.isInGrid(localCell)) {
					//this outlet is located in this tile
					wsGrid(localCell) = index;
					if (!wsGrid.isOnBorder(localCell))
						interiorLocalOutlets[localCell] = index;
				}
			}

			SolveLocal solveLocal;
			auto solu = solveLocal.solve(dirGrid, wsGrid);
			solu.interiorLocalOutlets = std::move(interiorLocalOutlets);
			gridLocalSolutions(gridCell) = std::move(solu);
			store.put(gridCell, std::move(dirGrid));
	}

	SolveGlobal globalSolution;
	globalSolution.gridInfo = gridInfo;
	globalSolution.gridLocalSolutions = std::move(gridLocalSolutions);
	globalSolution.solve();

	for (int i = 0; i < allTileFiles.size(); i++)
	{
		Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
		path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
		if (!exists(tilePath)) continue;

		Grid<FlowDir> dirGrid = store.get(gridCell, tilePath);
		Grid<int> wsGrid(dirGrid);
		wsGrid.allocate();

		SolveLocal localSolution;
		localSolution.updateBorderCellLabel(globalSolution.gridLocalSolutions(gridCell), globalSolution.gridLocalSolutions(gridCell).interiorLocalOutlets,wsGrid);
		localSolution.finalize(dirGrid, wsGrid);
		writeRaster(wsGrid, wsOutputFolder / (gridCell.to_string() + ".tif"));
	}
}
void tiled_ws_openmp(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets, Retention retention)
{
	initializeGDALForOpenMP();

	GridInfo gridInfo;
	//read gridInfo
	gridInfo.read(dirFileFolder / "gridInfo.txt");
	std::vector<path> allTileFiles = readAllTileFiles(dirFileFolder / "tiles.txt");
	TileFlowStore store(retention);

	Grid<LocalSolution> gridLocalSolutions(gridInfo.gridHeight, gridInfo.gridWidth);
	gridLocalSolutions.allocate();

	double step1_compute_max_time = 0;

	//#pragma omp parallel for
	#pragma omp parallel
	{
		#pragma omp for nowait
		for (int i = 0; i < allTileFiles.size(); i++)
		{
			Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
			path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
			if (!exists(tilePath)) continue;

			Grid<FlowDir> dirGrid;
			dirGrid = readRasterOpenMP<FlowDir>(tilePath);

			//set global outlet
			Grid<int> wsGrid(dirGrid);
			wsGrid.allocate();
			std::map <Cell, int> interiorLocalOutlets;
			for (auto& [cell, index] : outlets) {
				auto localCell = cell - Cell(gridCell.row * gridInfo.tileHeight, gridCell.col * gridInfo.tileWidth);
				if (wsGrid.isInGrid(localCell)) {
					//this outlet is located in this tile
					wsGrid(localCell) = index;
					if (!wsGrid.isOnBorder(localCell))
						interiorLocalOutlets[localCell] = index;
				}
			}

			SolveLocal solveLocal;
			auto solu = solveLocal.solve(dirGrid, wsGrid);
			solu.interiorLocalOutlets = std::move(interiorLocalOutlets);
			gridLocalSolutions(gridCell) = std::move(solu);
			store.put(gridCell, std::move(dirGrid));
		}
	}
	
	SolveGlobal globalSolution;
	globalSolution.gridInfo = gridInfo;
	globalSolution.gridLocalSolutions = std::move(gridLocalSolutions);
	globalSolution.solve();

	//for (int r = 0; r < gridInfo.gridHeight; r++)
	//	for (int c = 0; c < gridInfo.gridWidth; c++)
	//#pragma omp parallel for
	double step2_compute_max_time = 0;

	#pragma omp parallel
	{
		#pragma omp for nowait
		for (int i = 0; i < allTileFiles.size(); i++)
		{
			Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
			path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
			if (!exists(tilePath)) continue;

			Grid<FlowDir> dirGrid;
			dirGrid = store.getOpenMP(gridCell, tilePath);

			Grid<int> wsGrid(dirGrid);
			wsGrid.allocate();

			SolveLocal localSolution;
			localSolution.updateBorderCellLabel(globalSolution.gridLocalSolutions(gridCell), globalSolution.gridLocalSolutions(gridCell).interiorLocalOutlets, wsGrid);
			localSolution.finalize(dirGrid, wsGrid);

			writeRasterOpenMP(wsGrid, wsOutputFolder / (gridCell.to_string() + ".tif"));
		}
	}
}





