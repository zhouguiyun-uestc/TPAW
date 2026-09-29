#include "tool.h"
#include "ParaWatershed/ParaWatershed.h"
#include <Grid/grid.h>
#include <Grid/io_gdal.h>
#include <FastWatershed/WatershedFlowPathTraversal.h>
#include <Grid/tool.h>
#include <Grid/util.h>
void testAnsai_GenerateTiles()
{
	generateTiles("E:\\DEM\\flowdirs\\ansai_flow.tif", 300, 300, "E:\\DEM\\ansai_para\\flowdirs");
}

void testAnsaiSubset_GenerateTiles()
{
	generateTiles("E:\\DEM\\test_para\\ansai_10_10_dir.tif", 7, 4, "E:\\DEM\\test_para\\flowtiles");
}

void testAnsai_singleSolution()
{
	Grid<FlowDir> dirGrid = readRaster<FlowDir>("E:\\DEM\\flowdirs\\ansai_flow.tif");
	Grid<int> wsGrid(dirGrid);
	wsGrid.allocate();
	WatershedFlowPathTraversal(dirGrid, wsGrid);
	writeRaster(wsGrid, "E:\\DEM\\tiledWatersheds\\total.tif");
}

void testSmallDEM()
{
	Grid<float> dem(10, 10);
}

void GetSubset(const path& grandDirPath, const Cell& topLeft, int height, int width, const path& rootFolder, const path& datasetName)
{
	auto grid = readRaster<FlowDir>(grandDirPath);
	Grid<FlowDir> gridSubset(height, width);
	gridSubset.allocate();
	for (int r = 0; r < height; r++)
		for (int c = 0; c < width; c++) {
			gridSubset(Cell(r, c)) = grid(topLeft + Cell(r, c));
		}
	gridSubset.GeoTransform()[1] = 5;
	gridSubset.GeoTransform()[5] = -5;
	writeRaster(gridSubset, rootFolder / datasetName/"total_dir.tif");
}

void half(std::map<Cell, int>& outlets) {
	std::map<Cell, int> outlet_out;
	int i = 0;
	for (auto& [cell, index] : outlets)
	{
		i++;
		if (i % 2 != 0) continue;
		outlet_out[cell] = index;
	}
	outlets = outlet_out;
}

//检查parallel算法与串行算法结果是否一致, 中间生成分块水流方向
bool testParallel(const path& rootFolder, const path& datasetName, int tileHeight, int tileWidth, bool isGenerateTilesAndTotalWS)
{
	try
	{		
		path datasetFolder = rootFolder / datasetName;
		auto outlets = getGlobalOutlet(datasetFolder / "total_dir.tif");

		half(outlets);
		if (outlets.size() == 0) {
		//	std::cout << "no outlet is specified. " << std::endl;
			return true;
		}

		if (isGenerateTilesAndTotalWS) {			
			{
				remove_all(datasetFolder / "flowtiles");
				create_directory(datasetFolder / "flowtiles");
				remove(datasetFolder / "total_ws.tif");
				generateTiles(datasetFolder / "total_dir.tif", tileHeight, tileWidth, datasetFolder / "flowtiles");
				
				//使用串行数据生成流域
				Grid<FlowDir> dirGrid = readRaster<FlowDir>(datasetFolder / "total_dir.tif");
				Grid<int> wsGrid(dirGrid);
				wsGrid.allocate();
				WatershedFlowPathTraversal(dirGrid, wsGrid, outlets);
				writeRaster(wsGrid, datasetFolder / "total_ws.tif");
			}
		}
		writeOutletFile(outlets, datasetFolder / "flowtiles" / "outlets.txt");

		remove_all(datasetFolder / "wstiles");
		create_directory(datasetFolder / "wstiles");
		//tiled_ws_openmp(datasetFolder/"flowtiles", datasetFolder / "wstiles", outlets);
		tiled_ws_serial(datasetFolder / "flowtiles", datasetFolder / "wstiles", outlets);

		GridInfo gridInfo;
		//read gridInfo
		gridInfo.read(datasetFolder / "flowtiles" / "gridInfo.txt");
		mergeTiles<int>(gridInfo, datasetFolder / "wstiles", datasetFolder / "wstiles" / "merge.tif");
		auto grid1 = readRaster<int>(datasetFolder / "wstiles" / "merge.tif");
		auto grid2 = readRaster<int>(datasetFolder / "total_ws.tif");
		if (compareGrids(grid1, grid2)) {
			std::cout << "same " << std::endl;
			return true;
		}
		else {
			std::cout << "different " << std::endl;
			return false;
		}
	}
	catch (exception& ex) {
		std::cout << ex.what() << std::endl;
		return false;
	}
}

void testParallelBatch()
{
	for(int r=0; r<500;r++ )
		for (int c = 0; c < 470; c++) {
			std::cout << "top row:" << r << ", top col" << c << std::endl;
			GetSubset("E:\\DEM\\flowdirs\\ansai_flow.tif", Cell(r, c), 10, 10, "E:\\DEM\\parallel_test", "ansai_10by10");
			if (!testParallel("E:\\DEM\\parallel_test", "ansai_10by10", 4, 6,true))
			{
				std::cout <<"top row:"<< r <<", top col"<<c<< std::endl;
				return;
			}
		}
}

void testParallel_TopLeft_0_99()
{
	int r = 0;
	int c = 99;
	std::cout << "top row:" << r << ", top col" << c << std::endl;
	GetSubset("E:\\DEM\\flowdirs\\ansai_flow.tif", Cell(r, c), 10, 10, "E:\\DEM\\parallel_test", "ansai_10by10");
	if (!testParallel("E:\\DEM\\parallel_test", "ansai_10by10", 4, 6, true))
	{
		std::cout << "top row:" << r << ", top col" << c << std::endl;
		return;
	}
}


int main(int argc, char* argv[])
{
	//testParallel_TopLeft_0_99();
	//testParallelBatch();
	testParallel("E:\\DEM\\parallel_test","ansai", 80, 100,true);

	return 0;
}