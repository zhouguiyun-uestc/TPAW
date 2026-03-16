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

void tiled_ws_serial(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets)
{
	GridInfo gridInfo;
	//read gridInfo
	gridInfo.read(dirFileFolder / "gridInfo.txt");
	std::vector<path> allTileFiles=readAllTileFiles(dirFileFolder / "tiles.txt");

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

		Grid<FlowDir> dirGrid = readRaster<FlowDir>(tilePath);
		Grid<int> wsGrid(dirGrid);
		wsGrid.allocate();

		SolveLocal localSolution;
		localSolution.updateBorderCellLabel(globalSolution.gridLocalSolutions(gridCell), globalSolution.gridLocalSolutions(gridCell).interiorLocalOutlets,wsGrid);
		localSolution.finalize(dirGrid, wsGrid);
		writeRaster(wsGrid, wsOutputFolder / (gridCell.to_string() + ".tif"));
	}
}
void tiled_ws_openmp(const path& dirFileFolder, const path& wsOutputFolder, const std::map<Cell, int>& outlets)
{
	GridInfo gridInfo;
	//read gridInfo
	gridInfo.read(dirFileFolder / "gridInfo.txt");
	std::vector<path> allTileFiles = readAllTileFiles(dirFileFolder / "tiles.txt");


	Grid<LocalSolution> gridLocalSolutions(gridInfo.gridHeight, gridInfo.gridWidth);
	gridLocalSolutions.allocate();

	double step1_compute_max_time = 0;

	//#pragma omp parallel for
	#pragma omp parallel
	{
		TimeSpan ioSpan;
		TimeSpan computeSpan;

		computeSpan.reset();
		#pragma omp for nowait
		for (int i = 0; i < allTileFiles.size(); i++)
		{
			Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
			path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
			if (!exists(tilePath)) continue;

			Grid<FlowDir> dirGrid;
			ioSpan.reset();
			#pragma omp critical(read)
			{
				dirGrid = readRaster<FlowDir>(tilePath);
			}
			ioSpan.elapsed_millseconds();

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
		}
		computeSpan.elapsed_millseconds();
		computeSpan.total -= ioSpan.total;

		#pragma omp critical
		{
			if (computeSpan.total > step1_compute_max_time)
				step1_compute_max_time = computeSpan.total;
		}
	}
	
	TimeSpan globalcomputeSpan;

	globalcomputeSpan.reset();
	SolveGlobal globalSolution;
	globalSolution.gridInfo = gridInfo;
	globalSolution.gridLocalSolutions = std::move(gridLocalSolutions);
	globalSolution.solve();
	globalcomputeSpan.elapsed_millseconds();

	//for (int r = 0; r < gridInfo.gridHeight; r++)
	//	for (int c = 0; c < gridInfo.gridWidth; c++)
	//#pragma omp parallel for
	double step2_compute_max_time = 0;

	#pragma omp parallel
	{
		TimeSpan ioSpan;
		TimeSpan computeSpan;

		computeSpan.reset();
		#pragma omp for nowait
		for (int i = 0; i < allTileFiles.size(); i++)
		{
			Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
			path tilePath = dirFileFolder / (gridCell.to_string() + ".tif");
			if (!exists(tilePath)) continue;

			Grid<FlowDir> dirGrid;
			ioSpan.reset();
			#pragma omp critical(read)
			{
				dirGrid = readRaster<FlowDir>(tilePath);
			}
			ioSpan.elapsed_millseconds();

			Grid<int> wsGrid(dirGrid);
			wsGrid.allocate();

			SolveLocal localSolution;
			localSolution.updateBorderCellLabel(globalSolution.gridLocalSolutions(gridCell), globalSolution.gridLocalSolutions(gridCell).interiorLocalOutlets, wsGrid);
			localSolution.finalize(dirGrid, wsGrid);

			ioSpan.reset();
			#pragma omp critical(write)
			{
				writeRaster(wsGrid, wsOutputFolder / (gridCell.to_string() + ".tif"));
			}
			ioSpan.elapsed_millseconds();
		}
		computeSpan.elapsed_millseconds();
		computeSpan.total -= ioSpan.total;

		#pragma omp critical
		{
			if (computeSpan.total > step2_compute_max_time)
				step2_compute_max_time = computeSpan.total;
		}
	}

	double total_compute_max_time = step1_compute_max_time + step2_compute_max_time + globalcomputeSpan.total;
	std::cout << "Compute max Time: " << total_compute_max_time << endl;
}





