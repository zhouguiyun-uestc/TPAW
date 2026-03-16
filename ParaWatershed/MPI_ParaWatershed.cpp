#include "MPI_ParaWatershed.h"
#include "MPI_Communicate.h"
#include "SolveLocal.h"
#include "SolveGlobal.h"
#include <Grid/io_gdal.h>
#include <Grid/util.h>
#include <mpi.h>
#include "tool.h"
#include <Grid/tool.h>

using namespace std::filesystem;
void  mainProcess(
	int totalProcessCount,
	const std::filesystem::path& dirTileFolder, 
	int tileCount,
	GridInfo& gridInfo)
{
	TimeSpan timeSpan;
//	std::cout << "step1-main" << endl;
	Grid<LocalSolution> gridLocalSolutions(gridInfo.gridHeight,gridInfo.gridWidth);
	gridLocalSolutions.allocate();

	size_t totalTransferredByte = 0;
	//receive local solutions
	for (int i = 0; i < tileCount; i++) {
		auto solu=receive(-1);
		totalTransferredByte += solu.transferredByte;
		gridLocalSolutions(solu.gridCell) = solu;
	}
	
	std::cout << "main process computes global solutions" << endl;

//	std::cout << "step2-main"  << endl;
// 
	TimeSpan computeSpan;
	//compute
	SolveGlobal globalSolution;
	globalSolution.gridInfo = gridInfo;
	globalSolution.gridLocalSolutions = std::move(gridLocalSolutions);
	globalSolution.solve();
	auto computeTime = computeSpan.elapsed_millseconds();
	//send global solution to compute processes
	for (int r = 0; r < gridInfo.gridHeight; r++)
	for (int c = 0; c < gridInfo.gridWidth; c++)
	{
		Cell gridCell(r, c);
		auto& localSol = globalSolution.gridLocalSolutions(gridCell);

		if (localSol.rank == -1) continue;
//		std::cout << "send global solution to rank:" << localSol.rank<<endl;
		send(localSol, localSol.rank);
		totalTransferredByte += localSol.transferredByte;
	}

	double span = 0,io=0,comm=0;
	double spanMax=0, spanMin=0,io_sum = 0, comm_sum = 0;
	long vmpeak = 0, vmhwm = 0;
	ProcessMemUsage(vmpeak, vmhwm);
	long vmpeakMax = 0, vmhwmMax = 0;
	span = 0;
	MPI_Reduce(&span, &spanMax, 1, MPI_DOUBLE, MPI_MAX, 0,MPI_COMM_WORLD);
	span = std::numeric_limits<double>::max();
	MPI_Reduce(&span, &spanMin, 1, MPI_DOUBLE, MPI_MIN, 0,MPI_COMM_WORLD);
	MPI_Reduce(&io, &io_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&comm, &comm_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&vmpeak, &vmpeakMax, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&vmhwm, &vmhwmMax, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
	auto elapsedTime = timeSpan.elapsed_millseconds();
	auto io_avg = io_sum / (totalProcessCount - 1);
	auto comm_avg = comm_sum / (totalProcessCount - 1);
	vmpeakMax = vmpeakMax / 1024;
	vmhwmMax = vmhwmMax / 1024;
	std::cout << "Time in miliseconds by main processes: " << elapsedTime << endl;
	std::cout << "Time for solving global graph by main processes: " << computeTime << endl;
	std::cout << "Total communication bytes: " << totalTransferredByte<< endl;
	std::cout << "Consumer max Time: " << spanMax << endl;
	std::cout << "Consumer min Time: " << spanMin << endl;
	std::cout << "IO Time: " << io_avg << endl;
	std::cout << "Comm Time: " << comm_avg << endl;
	std::cout << "VmPeak(MB): " << vmpeakMax << endl;
	std::cout << "VmHwm(MB): " << vmhwmMax << endl;
	std::cout << "main process exits" << endl;
}

void computeProcess(
	int rank, //rank of compute process
	int totalProcessCount, //total process count, including main process
	const std::filesystem::path& dirTileFolder,
	const std::filesystem::path& wsTileFolder, 
	std::vector<std::filesystem::path>& allTileFiles, 
	GridInfo& gridInfo,
	std::map<Cell, int>& globalOutlets)
{
	//determine tiles to be processed by this process
	int computeProcessCount = totalProcessCount - 1;
	std::vector<int> tileIndices;
	for (int i = 0; i < allTileFiles.size(); i++) {
		//process rank starts from 1
		if (i % computeProcessCount == rank-1) tileIndices.push_back(i);
	}
	TimeSpan consumerSpan;
	TimeSpan ioSpan;

	TimeSpan commSpan;
	std::cout << "tile number processed by rank " << rank <<": " <<tileIndices.size()<< endl;
	std::map<Cell,std::map <Cell, int>> mapInterialLocalOutlets;
	//compute initial local solutions
	for (auto i : tileIndices) {
		//cout << allTileFiles[i] << endl;
		
		Cell gridCell = Cell::fromString(allTileFiles[i].stem().string());
		path tilePath = dirTileFolder / (gridCell.to_string() + ".tif");
		if (!exists(tilePath))
		{
			std::cout << "missing direction tile:" << tilePath << endl;
			return;
		}
		ioSpan.reset();
		Grid<FlowDir> dirGrid = readRaster<FlowDir>(tilePath,false);
		ioSpan.elapsed_millseconds();
		
		
		if (dirGrid.width() < 1 || dirGrid.height() < 1) {
			throw std::runtime_error("Both the height and width of the input flow dirtion tiles must be greater than 1.");
		}

		//set global outlets
		Grid<int> wsGrid(dirGrid);
		wsGrid.allocate(0);

		std::map <Cell, int> interiorLocalOutlets;
		for (auto& [cell, index] : globalOutlets) {

			auto localCell = cell - Cell(gridCell.row * gridInfo.tileHeight, gridCell.col * gridInfo.tileWidth);
			if (wsGrid.isInGrid(localCell)) {
				//this outlet is located in this tile
				wsGrid(localCell) = index;
				//if (!wsGrid.isOnBorder(localCell))
				//	interiorLocalOutlets[localCell] = index;
				if (!wsGrid.isOnBorder(localCell))
					mapInterialLocalOutlets[gridCell][localCell] = index;
			}
		}

		//find inital local solution
		SolveLocal solveLocal;
		auto solu = solveLocal.solve(dirGrid, wsGrid);
		//solu.interiorLocalOutlets = std::move(interiorLocalOutlets);
		solu.gridCell = gridCell;
		solu.rank = rank;
		
		//send this local solution to node 0
		commSpan.reset();
		send(solu, 0);
		commSpan.elapsed_millseconds();
	}
	auto spanStage1 = consumerSpan.elapsed_millseconds();

	std::cout << "rank " << rank << " computed assigned local solutions: " << tileIndices.size() << endl;

	consumerSpan.reset();
	//receive global solutions in the form of updated local solutions
	std::map<Cell, LocalSolution> localSolutions;
	for(int i=0; i< tileIndices.size(); i++)
	{
		commSpan.reset();
		auto localSol=receive(0);
		commSpan.elapsed_millseconds();
		localSolutions[localSol.gridCell] = std::move(localSol);
	}

	for (auto& [gridCell, localSolution] : localSolutions) {
		path tilePath = dirTileFolder / (gridCell.to_string() + ".tif");
		if (!exists(tilePath)) continue;
		
		ioSpan.reset();
		Grid<FlowDir> dirGrid = readRaster<FlowDir>(tilePath,false);
		ioSpan.elapsed_millseconds();
		
		Grid<int> wsGrid(dirGrid);
		wsGrid.allocate(0);

		SolveLocal localSolve;
		localSolve.updateBorderCellLabel(localSolutions.at(gridCell), mapInterialLocalOutlets[gridCell],wsGrid);
		localSolve.finalize(dirGrid, wsGrid);

		ioSpan.reset();
		writeRaster(wsGrid, wsTileFolder / (gridCell.to_string() + ".tif"));
		ioSpan.elapsed_millseconds();
	}
	auto spanStage2 = consumerSpan.elapsed_millseconds();
	double io = ioSpan.total;
	double comm = commSpan.total;
	double span = spanStage1 + spanStage2- io;
	long vmpeak = 0, vmhwm = 0;
	ProcessMemUsage(vmpeak, vmhwm);
	MPI_Reduce(&span, nullptr, 1, MPI_DOUBLE, MPI_MAX, 0,MPI_COMM_WORLD);
	MPI_Reduce(&span, nullptr, 1, MPI_DOUBLE, MPI_MIN, 0,MPI_COMM_WORLD);
	MPI_Reduce(&io, nullptr, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&comm, nullptr, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&vmpeak, nullptr, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&vmhwm, nullptr, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

	std::cout << "Rank " << rank << "comuted watersheds and exits " << endl;
} 
