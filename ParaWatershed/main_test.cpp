#include "tool.h"
#include "SolveLocal.h"
#include <iostream>
#include <Grid/io_gdal.h>
#include <Grid/FillDEMandComputeFlowDir.h>

void test_border_iteration()
{
	Grid<FlowDir> dirGrid(4, 4);
	NextBorderCellIter borderIter1(dirGrid);
	Cell c1;
	while (borderIter1.Next(c1))
	{
		std::cout << c1.to_string() << std::endl;
	}
}

void GetSubsetOfAnsai()
{
	auto grid=readRaster<FlowDir>("E:\\DEM\\flowdirs\\ansai_flow.tif");
	int height = 10;
	int width = 10;
	Cell topLeft(100, 100);
	Grid<FlowDir> gridSubset(height, width);
	gridSubset.allocate();
	for(int r=0; r<height; r++)
		for (int c = 0; c < width; c++) {
			gridSubset(Cell(r, c)) = grid(topLeft + Cell(r, c));
		}
	gridSubset.GeoTransform()[1] = 5;
	gridSubset.GeoTransform()[5] = -5;
	writeRaster(gridSubset, "E:\\DEM\\test_para\\ansai_10_10_dir.tif");
	//FillDEMandComputeFlowDir("E:\\DEM\\dems\\ansai_10_10.tif", "E:\\DEM\\dems\\ansai_10_10_dir.tif");
}

int main(int argc, char* argv[])
{
	Grid<FlowDir> g(10, 10);
	g.setNoDataValue(0);
	return 0;

	GetSubsetOfAnsai();
	//generateTiles("E:\\DEM\\flowdirs\\beijing_flow.tif", 300, 300, "E:\\DEM\\tiledflowdirs");
	return 0;
}