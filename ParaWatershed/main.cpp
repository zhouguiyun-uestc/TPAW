#include "tool.h"
#include "SolveLocal.h"
#include <iostream>
#include <Grid/io_gdal.h>
#include <Grid/FillDEMandComputeFlowDir.h>
#include <filesystem>
#include <FastWatershed/WatershedFlowPathTraversal.h>
#include <Grid/tool.h>
#include <Grid/util.h>
#include <omp.h>
#include "ParaWatershed.h"
#include "MPI_ParaWatershed.h"
using namespace std::filesystem;
int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << R"(ParaWatershed delineates watersheds using our proposed parallel parallel algorithm.
		ParaWatershed also generate tiles from a single DEM and merge tiles into a single DEM. 
		ParaWatershed also generate outlets from a single DEM." 
		Usage: ParaWatershed [method] {other parameters}." 
		[method] can be 'split','merge','generateOutlets', 'compare','diff','OpenMP', 'MPI'" 

		Example Usage 1: ParaWatershed split flowdir.tif  100 100 pathToOutputFolder
				split flowdir.tif into tiles of size of 100 by 100. 

		Example Usage 2: ParaWatershed merge pathToInputFlowDirFolder pathToInputWatershedFolder output.tif
				merge watershed tiles in the folder of pathToInputFolder to a single file output.tif

		Example Usage 3: ParaWatershed outlets  input_flowdir.tif output_outlets.txt 255
				automatically compute the outlets of up to 255 largest watersheds

		Example Usage 3: ParaWatershed compare  grid1.tif grid2.tif
				compare the give two grids

		Example Usage 4: ParaWatershed diff  grid1.tif grid2.tif diff.tif
				compute difference grid of grid1 and grid2

		Example Usage 5: ParaWatershed OpenMP 8 pathToInputFlowDirectionFolder pathToOutputWatershedFolder 
				use 8 OpenMP threads for computing watersheds given tiled flow direction folder

		Example Usage 6: mpiexec -np 3 ParaWatershed MPI pathToInputFlowDirectionFolder pathToOutputWatershedFolder
				use 3 MPI processes to compute watersheds given tiled flow direction folder
		)" << endl;
		return 0;
	}

	try
	{
		string method = argv[1];
		if (method == "split") {
			//Example Usage 1: ParaWatershed split dem.tif  100 100 pathToOutputFolder
			//	split dem.tif into tiles of size of 100 by 100
			if (argc < 6) {
				cout << "too few parameters for method 'split'" << endl;
				return 1;
			}

			path inputFlowDirPath = argv[2];
			int tileHeight = std::stoi(argv[3]);
			int tileWidth = std::stoi(argv[4]);
			path outputTiledFlowDirFloder = argv[5];

			generateTiles(inputFlowDirPath, tileHeight, tileWidth, outputTiledFlowDirFloder);
		}
		else if (method == "merge") {
			//Example Usage 2: ParaWatershed merge pathToInputFolder output.tif
			//merge watershed tiles in the folder of pathToInputFolder to a single file output.tif
			if (argc < 5) {
				cout << "too few parameters for method 'merge'" << endl;
				return 1;
			}
			path inputTiledFlowDirFolder = argv[2];
			path inputTiledWatershedFolder = argv[3];
			path outputMergedWatershedFile = argv[4];
			GridInfo gridInfo;
			//read gridInfo
			gridInfo.read(inputTiledFlowDirFolder / "gridInfo.txt");
			mergeTiles<int>(gridInfo, inputTiledWatershedFolder, outputMergedWatershedFile);
		}
		else if (method == "outlets") {
			//Example Usage 3: ParaWatershed outlets  input_flowdir.tif output_outlets.txt 255
			//	automatically compute the outlets of up to 255 largest watersheds

			if (argc < 5) {
				cout << "too few parameters for method 'outlets'" << endl;
				return 1;
			}

			path inputFlowDirPath = argv[2];
			path outputOutletPath = argv[3];
			int maxOutletCount = std::stoi(argv[4]);
			cout << "Max Outlet Count:" << maxOutletCount << endl;
			cout << "Reading flow dir grid" << endl;
			Grid<FlowDir> dirGrid = readRaster<FlowDir>(inputFlowDirPath);
			Grid<int> wsGrid(dirGrid);	

			if (maxOutletCount>0) 
				wsGrid.allocate(0);

			auto key2Cell = WatershedFastForOutletFiles(dirGrid, wsGrid,maxOutletCount);
			std::map<int, int> stats;
			if (maxOutletCount>0)
				stats= computeStats(wsGrid, maxOutletCount);
			cout << "Writing outlet file" << endl;
			writeOutletFile(stats, key2Cell, outputOutletPath); //rows and cols of outlets in files are 1-based
		}
		else if (method == "OpenMP") {
			//Example Usage 4: ParaWatershed OpenMP 8 pathToInputFlowDirectionFolder pathToOutputWatershedFolder
			//	use 8 OpenMP threads for computing watersheds given tiled flow direction folder

			int threadNumber = std::stoi(argv[2]);
			if (threadNumber <= 0) {
				std::cout << "Thread Number must be greater than 0" << threadNumber << std::endl;
				return 1;
			}

			path dirTileFolder = argv[3];
			path wsTileFolder = argv[4];
			if (!exists(dirTileFolder)) {
				std::cout << "Input tiled flow dir folder does not exist:" << dirTileFolder << std::endl;
				return 0;
			}

			//clear output folder 
			if (exists(wsTileFolder))
				remove_all(wsTileFolder);
			create_directories(wsTileFolder);

			std::map<Cell, int> globalOutlets = loadOutletLocations(dirTileFolder / "outlets.txt");

			omp_set_num_threads(threadNumber);
			std::cout << "Thread Number:" << threadNumber << std::endl;
			tiled_ws_openmp(dirTileFolder, wsTileFolder, globalOutlets);
		}
		else if (method == "MPI") {
			mpi_main(argc, argv);
		}
		else if (method == "diff") {
			if (argc < 5) {
				cout << "too few parameters for method 'diff'" << endl;
				return 1;
			}
			path path1 = argv[2];
			path path2 = argv[3];
			if (argc < 5) {
				cout << "too few parameters for method 'diff'" << endl;
				return 1;
			}

			path WSPath1 = argv[2];
			path WSPath2 = argv[3];
			path outputPath = argv[4];
			bool isSame = false;
			auto dataType = readRasterDataType(WSPath1);
			if (dataType == GDALDataType::GDT_Int32) {
				auto grid1 = readRaster<int>(WSPath1);
				auto grid2 = readRaster<int>(WSPath2);
				auto grid=diffGrids(grid1, grid2);
				writeRaster(grid, outputPath);
			}
			else if (dataType == GDALDataType::GDT_Byte) {
				auto grid1 = readRaster<unsigned char>(WSPath1);
				auto grid2 = readRaster<unsigned char>(WSPath2);
				auto grid = diffGrids(grid1, grid2);
				writeRaster(grid, outputPath);
			}
			else if (dataType == GDALDataType::GDT_Float32) {
				auto grid1 = readRaster<float>(WSPath1);
				auto grid2 = readRaster<float>(WSPath2);
				auto grid = diffGrids(grid1, grid2);
				writeRaster(grid, outputPath);
			}
			else {
				std::cout << "Unsupported data type for comparison. " << std::endl;
				return 1;
			}
		}
		else if (method == "compare") {
			//compare two grids
			if (argc < 4) {
				cout << "too few parameters for method 'compare'" << endl;
				return 1;
			}

			path WSPath1 = argv[2];
			path WSPath2 = argv[3];
			bool isSame = false;
			auto dataType = readRasterDataType(WSPath1);
			if (dataType == GDALDataType::GDT_Int32) {
				auto grid1 = readRaster<int>(WSPath1);
				auto grid2 = readRaster<int>(WSPath2);
				isSame=compareGrids(grid1, grid2);
			}
			else if (dataType == GDALDataType::GDT_Byte) {
				auto grid1 = readRaster<unsigned char>(WSPath1);
				auto grid2 = readRaster<unsigned char>(WSPath2);
				isSame = compareGrids(grid1, grid2);
			}
			else if (dataType == GDALDataType::GDT_Float32) {
				auto grid1 = readRaster<float>(WSPath1);
				auto grid2 = readRaster<float>(WSPath2);
				isSame = compareGrids(grid1, grid2);
			}
			else {
				std::cout << "Unsupported data type for comparison. " << std::endl;
				return 1 ;
			}

			if (isSame) {
				std::cout << "same " << std::endl;
				return true;
			}
			else {
				std::cout << "different " << std::endl;
				return false;
			}
		}
		else {
			cout << "Unknown processing method!" << endl;
		}
	}
	catch (exception& ex) {
		cout << ex.what() << endl;
	}
	return 0;
}