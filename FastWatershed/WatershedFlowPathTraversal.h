#pragma once
#include <Grid/grid.h>
#include <map>
void WatershedFlowPathTraversal(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid, const std::map<Cell, int>& outlets= std::map<Cell, int>());
void FlowPathTraversal(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid, bool isNeedtoRelabelAllNegativeOne);
void WatershedFlowPathTraversal_OpenMP(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid, const std::map<Cell, int>& outlets = std::map<Cell, int>());
std::map<int, Cell> WatershedFastForOutletFiles(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid,int maxOutletCount);
