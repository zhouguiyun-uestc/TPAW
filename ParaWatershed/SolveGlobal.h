#pragma once
#include <map>
#include <Grid/flowDir.h>
#include <Grid/cell.h>
#include <Grid/grid.h>
#include "grid_info.h"
#include "SolveLocal.h"
class SolveGlobal
{
public:
	GridInfo gridInfo;
	
	Grid<LocalSolution> gridLocalSolutions;

public:
	void solve();
	bool moveToDownstreamGlobalCell(
		Cell& gridCell, //current tile in a grid
		Cell& srcCell);
	void computeTileDimension(const Cell& gridCell, int& height, int& width);
};